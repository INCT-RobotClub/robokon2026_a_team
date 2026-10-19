#ifndef INC_LED_H_
#define INC_LED_H_

#include "main.h"

#define LED_COUNT     140
#define PWM_0         34
#define PWM_1         67
#define RESET_SLOTS   280
#define BUFFER_SIZE   ((LED_COUNT * 24) + RESET_SLOTS)

// Single LED RGB and Brightness container
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t brightness; // Master brightness scale (0: OFF, 255: MAX)
} LED;

// Hardware driver functions
void LED_Init(TIM_HandleTypeDef *htim, uint32_t channel);
void LED_Show(LED *led_array, uint16_t count);

// Utility animation helpers (Non-blocking: advances 1 frame per call)
void LED_Anim_Rainbow(LED *led_array, uint16_t count, uint8_t step);
void LED_Anim_ShootingStar(LED *led_array, uint16_t count, int16_t *bullet_pos, uint8_t r, uint8_t g, uint8_t b);
void LED_Anim_Scanner(LED *led_array, uint16_t count, uint8_t r, uint8_t g, uint8_t b);
void LED_Anim_Breathing(LED *led_array, uint16_t count, uint8_t r, uint8_t g, uint8_t b);
void LED_Anim_ColorWipe(LED *led_array, uint16_t count, uint8_t r, uint8_t g, uint8_t b);
void LED_Anim_Sparkle(LED *led_array, uint16_t count, uint8_t r, uint8_t g, uint8_t b, uint8_t density);
void LED_Anim_Fire(LED *led_array, uint16_t count);

#endif /* INC_LED_H_ */
