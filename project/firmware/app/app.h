/**
 * @file app.h
 * @author Timon Burkard (timon.burkard@gwf.ch)
 *
 * @copyright Copyright (c) 2025 GWF AG
 *
 */

#ifndef APP_H_
#define APP_H_

#include <stdbool.h>
#include <stdint.h>

#include "max7219.h"

typedef enum button {
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_CENTER,
    BUTTON_NONE,
} button_t;

extern bool      matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT];
extern max7219_t max7219;

void     app(void);
void     app_beep(uint16_t duration_ms);
button_t app_get_user_input(void);
void     app_flash_init_highscore();
uint16_t app_flash_load_highscore();
void     app_flash_save_highscore(uint16_t score);
void     app_matrix_clean(bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT]);

#endif /* APP_H_ */
