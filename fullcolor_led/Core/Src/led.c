#include "led.h"

static TIM_HandleTypeDef *led_htim = NULL;
static uint32_t led_channel = 0;
static uint16_t pwm_buffer[BUFFER_SIZE];
static volatile uint8_t is_transmitting = 0;

static void HSV_To_RGB(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (s == 0) {
        *r = v; *g = v; *b = v;
        return;
    }
    uint8_t region = h / 43;
    uint8_t remainder = (uint8_t)((h - (region * 43)) * 6);
    uint8_t p = (uint8_t)((v * (255 - s)) >> 8);
    uint8_t q = (uint8_t)((v * (255 - ((s * remainder) >> 8))) >> 8);
    uint8_t t = (uint8_t)((v * (255 - ((s * (255 - remainder)) >> 8))) >> 8);

    switch (region) {
        case 0:  *r = v; *g = t; *b = p; break;
        case 1:  *r = q; *g = v; *b = p; break;
        case 2:  *r = p; *g = v; *b = t; break;
        case 3:  *r = p; *g = q; *b = v; break;
        case 4:  *r = t; *g = p; *b = v; break;
        default: *r = v; *g = p; *b = q; break;
    }
}

void LED_Init(TIM_HandleTypeDef *htim, uint32_t channel)
{
    led_htim = htim;
    led_channel = channel;
    for (uint16_t i = 0; i < BUFFER_SIZE; i++) {
        pwm_buffer[i] = 0;
    }
}

// Convert user LED array into WS2812B pulse stream
void LED_Show(LED *led_array, uint16_t count)
{
    if (is_transmitting || led_htim == NULL || led_array == NULL) return;

    uint16_t active_leds = (count > LED_COUNT) ? LED_COUNT : count;

    for (uint16_t i = 0; i < active_leds; i++) {
        // Apply individual LED brightness scale
        uint8_t r = (uint8_t)(((uint16_t)led_array[i].r * led_array[i].brightness) >> 8);
        uint8_t g = (uint8_t)(((uint16_t)led_array[i].g * led_array[i].brightness) >> 8);
        uint8_t b = (uint8_t)(((uint16_t)led_array[i].b * led_array[i].brightness) >> 8);

        // WS2812B bit stream: R -> G -> B
        uint32_t packet = ((uint32_t)g << 16) | ((uint32_t)b << 8) | (uint32_t)r;
        uint16_t offset = i * 24;

        for (int b_idx = 23; b_idx >= 0; b_idx--) {
            pwm_buffer[offset++] = (packet & (1UL << b_idx)) ? PWM_1 : PWM_0;
        }
    }

    // Trailing reset low duration
    for (uint16_t i = active_leds * 24; i < BUFFER_SIZE; i++) {
        pwm_buffer[i] = 0;
    }

    is_transmitting = 1;
    HAL_TIM_PWM_Start_DMA(led_htim, led_channel, (uint32_t *)pwm_buffer, BUFFER_SIZE);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == led_htim->Instance) {
        HAL_TIM_PWM_Stop_DMA(led_htim, led_channel);
        is_transmitting = 0;
    }
}

// Advance 1 frame of rainbow into user array
void LED_Anim_Rainbow(LED *led_array, uint16_t count, uint8_t step)
{
    static uint8_t hue_offset = 0;
    for (uint16_t i = 0; i < count; i++) {
        uint8_t hue = (uint8_t)(hue_offset + (i * (256 / count)));
        HSV_To_RGB(hue, 255, 255, &led_array[i].r, &led_array[i].g, &led_array[i].b);


    }
    hue_offset += step;
}

// Advance 1 frame of shooting star into user array
void LED_Anim_ShootingStar(LED *led_array, uint16_t count, int16_t *bullet_pos, uint8_t r, uint8_t g, uint8_t b)
{
    // Decay tail
    for (uint16_t i = 0; i < count; i++) {
        led_array[i].r = (uint8_t)((led_array[i].r * 180) >> 8);
        led_array[i].g = (uint8_t)((led_array[i].g * 180) >> 8);
        led_array[i].b = (uint8_t)((led_array[i].b * 180) >> 8);
    }

    // Move bullet head
    if (*bullet_pos >= 0 && *bullet_pos < count) {
        led_array[*bullet_pos].r = r;
        led_array[*bullet_pos].g = g;
        led_array[*bullet_pos].b = b;
        (*bullet_pos)++;
    } else {
        *bullet_pos = -1;
    }
}
// 1. Scanner (Knight Rider bounce effect)
void LED_Anim_Scanner(LED *led_array, uint16_t count, uint8_t r, uint8_t g, uint8_t b)
{
    static int16_t scan_pos = 0;
    static int8_t dir = 1;

    // Decay previous lights
    for (uint16_t i = 0; i < count; i++) {
        led_array[i].r = (uint8_t)((led_array[i].r * 150) >> 8);
        led_array[i].g = (uint8_t)((led_array[i].g * 150) >> 8);
        led_array[i].b = (uint8_t)((led_array[i].b * 150) >> 8);
    }

    if (scan_pos >= 0 && scan_pos < count) {
        led_array[scan_pos].r = r;
        led_array[scan_pos].g = g;
        led_array[scan_pos].b = b;
    }

    scan_pos += dir;
    if (scan_pos >= count - 1) {
        scan_pos = count - 1;
        dir = -1;
    } else if (scan_pos <= 0) {
        scan_pos = 0;
        dir = 1;
    }
}

// 2. Breathing / Energy Charge
void LED_Anim_Breathing(LED *led_array, uint16_t count, uint8_t r, uint8_t g, uint8_t b)
{
    static uint8_t breath_val = 0;
    static int8_t breath_step = 2;

    int16_t next_val = breath_val + breath_step;
    if (next_val >= 255) {
        next_val = 255;
        breath_step = -breath_step;
    } else if (next_val <= 0) {
        next_val = 0;
        breath_step = -breath_step;
    }
    breath_val = (uint8_t)next_val;

    for (uint16_t i = 0; i < count; i++) {
        led_array[i].r = r;
        led_array[i].g = g;
        led_array[i].b = b;
        led_array[i].brightness = breath_val;
    }
}

// 3. Color Wipe (Sequential filling)
void LED_Anim_ColorWipe(LED *led_array, uint16_t count, uint8_t r, uint8_t g, uint8_t b)
{
    static uint16_t fill_idx = 0;

    if (fill_idx < count) {
        led_array[fill_idx].r = r;
        led_array[fill_idx].g = g;
        led_array[fill_idx].b = b;
        fill_idx++;
    } else {
        // Reset and clear for looping demonstration
        fill_idx = 0;
        for (uint16_t i = 0; i < count; i++) {
            led_array[i].r = 0;
            led_array[i].g = 0;
            led_array[i].b = 0;
        }
    }
}

// 4. Sparkle / Cyberpunk glitch sparks
void LED_Anim_Sparkle(LED *led_array, uint16_t count, uint8_t r, uint8_t g, uint8_t b, uint8_t density)
{
    // High-speed decay
    for (uint16_t i = 0; i < count; i++) {
        led_array[i].r = (uint8_t)((led_array[i].r * 110) >> 8);
        led_array[i].g = (uint8_t)((led_array[i].g * 110) >> 8);
        led_array[i].b = (uint8_t)((led_array[i].b * 110) >> 8);
    }

    // Pseudo-random spark triggers
    static uint32_t seed = 0x12345678;
    for (uint8_t k = 0; k < density; k++) {
        seed = seed * 1664525UL + 1013904223UL;
        uint16_t target = (seed >> 16) % count;
        led_array[target].r = r;
        led_array[target].g = g;
        led_array[target].b = b;
    }
}

// 5. Fire / Plasma flame simulator
void LED_Anim_Fire(LED *led_array, uint16_t count)
{
    static uint8_t heat[LED_COUNT] = {0};
    static uint32_t seed = 0x87654321;

    // Step 1: Cool down every cell slightly
    for (uint16_t i = 0; i < count; i++) {
        seed = seed * 1664525UL + 1013904223UL;
        uint8_t cooldown = ((seed >> 16) % 15) + 2;
        if (cooldown > heat[i]) {
            heat[i] = 0;
        } else {
            heat[i] -= cooldown;
        }
    }

    // Step 2: Heat drifts up and diffuses
    for (int16_t k = count - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
    }

    // Step 3: Randomly ignite new sparks near bottom
    seed = seed * 1664525UL + 1013904223UL;
    if (((seed >> 16) % 255) < 160) {
        seed = seed * 1664525UL + 1013904223UL;
        uint8_t spark_heat = ((seed >> 16) % 95) + 160;
        heat[0] = (heat[0] + spark_heat) / 2;
    }

    // Step 4: Map heat to flame colors (Red -> Orange -> Yellow)
    for (uint16_t j = 0; j < count; j++) {
        uint8_t t = heat[j];
        if (t > 170) {
            led_array[j].r = 255;
            led_array[j].g = 255;
            led_array[j].b = (t - 170) * 3;
        } else if (t > 85) {
            led_array[j].r = 255;
            led_array[j].g = (t - 85) * 3;
            led_array[j].b = 0;
        } else {
            led_array[j].r = t * 3;
            led_array[j].g = 0;
            led_array[j].b = 0;
        }
    }
}
