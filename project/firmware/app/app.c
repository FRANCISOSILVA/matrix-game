/**
 * @file app.c
 * @author Timon Burkard (timon.burkard@gwf.ch)
 *
 * @copyright Copyright (c) 2025 GWF AG
 *
 */

#include "app.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "max7219.h"
#include "tictactoe.h"
#include "snake.h"
#include "ssd1306.h"

#define BUTTON_DEBOUNCE_DELAY_MS 10

#define GAME_OPTIONS_PER_SCREEN 3

extern SPI_HandleTypeDef hspi1;

typedef enum {
    SNAKE     = 0,
    TICTACTOE = 1,
    GAME_AMOUNT, // Keep at end!
} game_id_t;

typedef struct {
    game_id_t   id;
    const char* name;
    void (*run)(void);
} game_t;

/* clang-format off */

static game_t games[] = {
    [SNAKE]     = { .id = SNAKE,     .name = "Snake",     .run = snake     },
    [TICTACTOE] = { .id = TICTACTOE, .name = "TicTacToe", .run = tictactoe },
};

/* clang-format on */

bool      matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT] = { false };
max7219_t max7219                                           = { 0 };

void app_matrix_clean(bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT])
{
    for (uint8_t i = 0; i < MAX7219_COLUMN_AMOUNT; i++) {
        for (uint8_t j = 0; j < MAX7219_ROW_AMOUNT; j++) {
            matrix[i][j] = false;
        }
    }
}

button_t app_get_user_input(void)
{
    static bool button_up_previous     = 0;
    static bool button_down_previous   = 0;
    static bool button_left_previous   = 0;
    static bool button_right_previous  = 0;
    static bool button_center_previous = 0;

    button_t button = BUTTON_NONE;

    bool button_up_current     = HAL_GPIO_ReadPin(BUTTON_UP_GPIO_Port, BUTTON_UP_Pin);
    bool button_down_current   = HAL_GPIO_ReadPin(BUTTON_DOWN_GPIO_Port, BUTTON_DOWN_Pin);
    bool button_left_current   = HAL_GPIO_ReadPin(BUTTON_LEFT_GPIO_Port, BUTTON_LEFT_Pin);
    bool button_right_current  = HAL_GPIO_ReadPin(BUTTON_RIGHT_GPIO_Port, BUTTON_RIGHT_Pin);
    bool button_center_current = HAL_GPIO_ReadPin(BUTTON_CENTER_GPIO_Port, BUTTON_CENTER_Pin);

    if (button_up_current && !button_up_previous) {
        button = BUTTON_UP;
    }

    if (button_down_current && !button_down_previous) {
        button = BUTTON_DOWN;
    }

    if (button_left_current && !button_left_previous) {
        button = BUTTON_LEFT;
    }

    if (button_right_current && !button_right_previous) {
        button = BUTTON_RIGHT;
    }

    if (button_center_current && !button_center_previous) {
        button = BUTTON_CENTER;
    }

    button_up_previous     = button_up_current;
    button_down_previous   = button_down_current;
    button_left_previous   = button_left_current;
    button_right_previous  = button_right_current;
    button_center_previous = button_center_current;

    HAL_Delay(BUTTON_DEBOUNCE_DELAY_MS);

    return button;
}

void app_beep(uint16_t duration_ms)
{
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
    HAL_Delay(duration_ms);
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
}

void app_lcd_print_title(void)
{
    SSD1306_Clear();
    SSD1306_GotoXY(0, APP_LCD_ROW_TITLE);
    SSD1306_Puts(APP_LCD_TITLE, &Font_7x10, 1);
    SSD1306_GotoXY(0, APP_LCD_ROW_TITLE_SEPARATION);
    SSD1306_Puts(APP_LCD_TITLE_SEPARATION, &Font_7x10, 1);
    SSD1306_UpdateScreen();
}

static void lcd_print_game_selection(game_id_t game_id)
{
    // Clear all game options
    SSD1306_GotoXY(0, APP_LCD_ROW_GAME_NAME);
    SSD1306_Puts(APP_LCD_EMPTY_LINE, &Font_7x10, 1);
    SSD1306_GotoXY(0, APP_LCD_ROW_GAME_DYNAMIC_0);
    SSD1306_Puts(APP_LCD_EMPTY_LINE, &Font_7x10, 1);
    SSD1306_GotoXY(0, APP_LCD_ROW_GAME_DYNAMIC_1);
    SSD1306_Puts(APP_LCD_EMPTY_LINE, &Font_7x10, 1);

    game_id_t        start_id          = 0;
    static game_id_t start_id_previous = 0;

    // Check if currently selected game is still displayable with the previous start ID
    if ((start_id_previous <= game_id) && (game_id < (start_id_previous + GAME_OPTIONS_PER_SCREEN))) {
        start_id = start_id_previous;
    } else if (game_id < start_id_previous) {
        start_id = game_id;
    } else if (game_id >= (start_id_previous + GAME_OPTIONS_PER_SCREEN)) {
        start_id = game_id - (GAME_OPTIONS_PER_SCREEN - 1);
    }

    for (uint8_t i = 0; i < GAME_OPTIONS_PER_SCREEN; i++) {
        if ((start_id + i) >= GAME_AMOUNT) {
            break;
        }

        if ((start_id + i) == game_id) {
            SSD1306_GotoXY(0, APP_LCD_ROW_GAME_NAME + (i * APP_LCD_ROW_GAME_DIFFERENCE));
            SSD1306_Puts(">", &Font_7x10, 1);
        }

        SSD1306_GotoXY(APP_LCD_COL_GAME_SELECTION_INDENTATION, APP_LCD_ROW_GAME_NAME + (i * APP_LCD_ROW_GAME_DIFFERENCE));
        SSD1306_Puts(games[start_id + i].name, &Font_7x10, 1);
    }

    SSD1306_UpdateScreen();

    start_id_previous = start_id;
}

static game_id_t select_game(void)
{
    button_t  button  = BUTTON_NONE;
    game_id_t game_id = SNAKE;

    app_lcd_print_title();

    lcd_print_game_selection(game_id);

    do {
        do {
            button = app_get_user_input();
        } while (button == BUTTON_NONE);

        if (button == BUTTON_DOWN) {
            if (game_id < (GAME_AMOUNT - 1)) {
                ++game_id;
            }

            lcd_print_game_selection(game_id);
        }

        if (button == BUTTON_UP) {
            if (game_id > 0) {
                --game_id;
            }

            lcd_print_game_selection(game_id);
        }
    } while (button != BUTTON_CENTER);

    return game_id;
}

void app(void)
{
    game_id_t game_id;

    if (max7219_init(&max7219, &hspi1, MAX_SPI_CS_GPIO_Port, MAX_SPI_CS_Pin) != MAX7219_OK) {
        for (;;) {
        } // Error handling...
    }

    SSD1306_Init();

    for (;;) {
        app_matrix_clean(matrix);

        if (max7219_set_matrix(&max7219, matrix) != MAX7219_OK) {
            for (;;) {
            } // Error handling...
        }

        game_id = select_game();

        if (games[game_id].run != NULL) {
            games[game_id].run();
        }
    }
}
