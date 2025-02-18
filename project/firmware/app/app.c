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
#include "snake.h"
#include "ssd1306.h"

#define BEEP_SHORT_MS 75
#define BEEP_LONG_MS  750

#define BUTTON_DEBOUNCE_DELAY_MS 10

#define FLASH_HIGHSCORE_ADDRESS 0x08007F00 // Last page

extern SPI_HandleTypeDef hspi1;

bool      matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT] = { false };
max7219_t max7219                                           = { 0 };

/**
 * @brief Initialize the highscore in flash memory, if not yet done
 */
void app_flash_init_highscore()
{
    if (app_flash_load_highscore() == 0xFFFF) {
        app_flash_save_highscore(0);
    }
}

uint16_t app_flash_load_highscore()
{
    return *((uint16_t*)FLASH_HIGHSCORE_ADDRESS); // Read stored value
}

void app_flash_save_highscore(uint16_t score)
{
    HAL_FLASH_Unlock(); // Unlock flash for writing

    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t               PageError = 0;

    EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FLASH_HIGHSCORE_ADDRESS;
    EraseInitStruct.NbPages     = 1;
    HAL_FLASHEx_Erase(&EraseInitStruct, &PageError); // Erase before writing

    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, FLASH_HIGHSCORE_ADDRESS, score);

    HAL_FLASH_Lock(); // Lock flash after writing
}

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

void app(void)
{
    max7219_error_t error_code = MAX7219_OK;
    button_t        button     = BUTTON_NONE;
    button_t        direction  = BUTTON_RIGHT;
    move_t          move_state = MOVE_NORMAL;

    error_code = max7219_init(&max7219, &hspi1, MAX_SPI_CS_GPIO_Port, MAX_SPI_CS_Pin);

    if (error_code != MAX7219_OK) {
        for (;;) {
        } // Error handling...
    }

    app_flash_init_highscore();

    SSD1306_Init();
    snake_lcd_start();

    snake_init();

    for (;;) {
        while (app_get_user_input() == BUTTON_NONE) {
            // Wait for user to start the game
        }

        srand(HAL_GetTick());
        snake_lcd_start();
        snake_start_game(&direction);

        do {
            app_beep(BEEP_SHORT_MS);
            snake_food_generate();

            do {
                for (uint8_t i = 0; i < SNAKE_SEQUENCE_PERIOD_10MS; i++) {
                    button = app_get_user_input(); // Takes 10 ms

                    if (button != BUTTON_NONE) {
                        direction = button;
                    }
                }

                move_state = snake_move(direction);

                snake_convert_to_matrix(matrix);

                if (max7219_set_matrix(&max7219, matrix) != MAX7219_OK) {
                    for (;;) {
                    } // Error handling...
                }
            } while (move_state == MOVE_NORMAL);
        } while (move_state != MOVE_GAME_OVER);

        app_beep(BEEP_LONG_MS);
        snake_handle_score();
    }
}
