/**
 * @file drawing.c
 * @author Francisco Da Silva (francisco.dasilva@gwf.ch)
 *
 * @copyright Copyright (c) 2025 GWF AG
 *
 */

#include "drawing.h"

#include <stdio.h>
#include <stdbool.h>

#include "app.h"
#include "max7219.h"
#include "ssd1306.h"

typedef struct {
    uint8_t row;
    uint8_t col;
} cursor_t;

static void lcd_start(void)
{
    app_lcd_print_title();

    SSD1306_GotoXY(0, APP_LCD_ROW_GAME_NAME);
    SSD1306_Puts("Drawing", &Font_7x10, 1);
    SSD1306_GotoXY(0, APP_LCD_ROW_GAME_DYNAMIC_1);
    SSD1306_Puts("Press ok to end", &Font_7x10, 1);
    SSD1306_UpdateScreen();
}

void drawing(void)
{
    cursor_t cursor = { .row = 0, .col = 0 };

    app_matrix_clean(matrix);
    max7219_set_matrix(&max7219, matrix);

    lcd_start();

    while (1) {
        // read user button
        button_t button = app_get_user_input();

        // update cursor
        switch (button) {
        case BUTTON_RIGHT:
            if (cursor.col < MAX7219_COLUMN_AMOUNT - 1) {
                cursor.col++;
            }

            break;

        case BUTTON_LEFT:
            if (cursor.col > 0) {
                cursor.col--;
            }

            break;

        case BUTTON_UP:
            if (cursor.row > 0) {
                cursor.row--;
            }

            break;

        case BUTTON_DOWN:
            if (cursor.row < MAX7219_ROW_AMOUNT - 1) {
                cursor.row++;
            }

            break;

        case BUTTON_CENTER:
            return; // end

        default:
            break;
        }

        matrix[cursor.col][cursor.row] = true;

        // update matrix
        max7219_set_matrix(&max7219, matrix);
    }
}
