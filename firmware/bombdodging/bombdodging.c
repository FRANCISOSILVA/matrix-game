/**
 * @file    bombdodging.c
 * @author  Francisco Da Silva (francisco.dasilva@gwf.ch)
 * @copyright Copyright (c) 2025 GWF AG
 */

#include "bombdodging.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

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
    SSD1306_Puts("Bombdodging", &Font_7x10, 1);
    SSD1306_UpdateScreen();
}

void bombdodging(void) {
    cursor_t cursor = { .row = 0, .col = 0 };

    // Place bomb randomly
    cursor_t bomb;
    do {
        bomb.row = rand() % 8;
        bomb.col = rand() % 8;
    } while (bomb.row == cursor.row && bomb.col == cursor.col);

    // Fill matrix with false and keep visited count
    bool visited[8][8] = { false };
    int visited_count = 1; // start position is marked
    visited[cursor.row][cursor.col] = true;

    app_matrix_clean(matrix);
    matrix[cursor.col][cursor.row] = true;
    max7219_set_matrix(&max7219, matrix);

    lcd_start();

    while (1) {
        // Read user button
        button_t button = app_get_user_input();

        // Update cursor
        switch (button) {
            case BUTTON_RIGHT:
                if (cursor.col < MAX7219_COLUMN_AMOUNT - 1) cursor.col++;
                break;
            case BUTTON_LEFT:
                if (cursor.col > 0) cursor.col--;
                break;
            case BUTTON_UP:
                if (cursor.row > 0) cursor.row--;
                break;
            case BUTTON_DOWN:
                if (cursor.row < MAX7219_ROW_AMOUNT -1 ) cursor.row++;
                break;
            default:
                break;
        }

        // Check for collision with hidden bomb
        if (cursor.row == bomb.row && cursor.col == bomb.col) {
            // Game Over Text
            SSD1306_GotoXY(0, APP_LCD_ROW_GAME_NAME + 20);
            SSD1306_Puts("BOOOOM!!!", &Font_7x10, 1);
            SSD1306_GotoXY(0, APP_LCD_ROW_GAME_DYNAMIC_1);
            SSD1306_Puts("Game Over!", &Font_7x10, 1);
            SSD1306_UpdateScreen();
            app_beep(BEEP_LONG_MS);
            while (app_get_user_input() == BUTTON_NONE) {
                // Wait
            }
            return; // End game
        }

        // Mark cell as visited only if it wasn't before
        if (!visited[cursor.row][cursor.col]) {
            visited[cursor.row][cursor.col] = true;
            visited_count++;
        }

        // Draw player path
        matrix[cursor.col][cursor.row] = true;
        max7219_set_matrix(&max7219, matrix);

        // Check win
        if (visited_count == 63 && !visited[bomb.row][bomb.col]) {
            SSD1306_GotoXY(0, APP_LCD_ROW_GAME_NAME + 20);
            SSD1306_Puts("YOU WIN!", &Font_7x10, 1);
            SSD1306_UpdateScreen();
            app_beep(BEEP_LONG_MS);
            while (app_get_user_input() == BUTTON_NONE) {
                // Wait for user to press a button
            }
            return;
        }
    }
}
