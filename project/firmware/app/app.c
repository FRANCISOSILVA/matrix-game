/**
 * @file app.c
 * @author Timon Burkard (timon.burkard@gwf.ch)
 *
 * @copyright Copyright (c) 2025 GWF AG
 *
 */

#include <stdbool.h>

#include "main.h"
#include "max7219.h"
#include "ssd1306.h"

extern SPI_HandleTypeDef hspi1;

void app(void)
{
    // Test LCD
    SSD1306_Init();
    SSD1306_GotoXY(0, 0);
    SSD1306_Puts("GWF Schnupperlehre", &Font_7x10, 1);
    SSD1306_GotoXY(0, 10);
    SSD1306_Puts("-----------------", &Font_7x10, 1);
    SSD1306_GotoXY(0, 25);
    SSD1306_Puts("Hello World !!!", &Font_7x10, 1);
    SSD1306_UpdateScreen();

    max7219_t       max7219    = { 0 };
    max7219_error_t error_code = MAX7219_OK;

    error_code = max7219_init(&max7219, &hspi1, MAX_SPI_CS_GPIO_Port, MAX_SPI_CS_Pin);

    if (error_code != MAX7219_OK) {
        for (;;) {
        } // Error handling...
    }

    error_code = max7219_send(&max7219, MAX7219_ADR_SHUTDOWN, MAX7219_REG_SHUTDOWN_MODE_NORMAL);

    if (error_code != MAX7219_OK) {
        for (;;) {
        } // Error handling...
    }

    error_code = max7219_send(&max7219, MAX7219_ADR_SCAN_LIMIT, MAX7219_REG_SCAN_LIMIT_DIG_0_7);

    if (error_code != MAX7219_OK) {
        for (;;) {
        } // Error handling...
    }

    error_code = max7219_send(&max7219, MAX7219_ADR_INTENSITY, MAX7219_REG_INTENSITY_15_32);

    if (error_code != MAX7219_OK) {
        for (;;) {
        } // Error handling...
    }

    error_code = max7219_send(&max7219, MAX7219_ADR_DISPLAY_TEST, MAX7219_REG_DISPLAY_TEST_MODE_TEST);

    if (error_code != MAX7219_OK) {
        for (;;) {
        } // Error handling...
    }

    error_code = max7219_send(&max7219, MAX7219_ADR_DISPLAY_TEST, MAX7219_REG_DISPLAY_TEST_MODE_NORMAL);

    if (error_code != MAX7219_OK) {
        for (;;) {
        } // Error handling...
    }

    error_code = max7219_send(&max7219, MAX7219_COLUMN_0, 0x00);
    error_code = max7219_send(&max7219, MAX7219_COLUMN_1, 0x00);
    error_code = max7219_send(&max7219, MAX7219_COLUMN_2, 0x00);
    error_code = max7219_send(&max7219, MAX7219_COLUMN_3, 0x00);
    error_code = max7219_send(&max7219, MAX7219_COLUMN_4, 0x00);
    error_code = max7219_send(&max7219, MAX7219_COLUMN_5, 0x00);
    error_code = max7219_send(&max7219, MAX7219_COLUMN_6, 0x00);
    error_code = max7219_send(&max7219, MAX7219_COLUMN_7, 0x00);

    if (error_code != MAX7219_OK) {
        for (;;) {
        } // Error handling...
    }

    bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT] = { false };

    matrix[0][0] = true;
    matrix[0][1] = true;
    matrix[3][7] = true;

    error_code = max7219_set_matrix(&max7219, matrix);

    if (error_code != MAX7219_OK) {
        for (;;) {
        } // Error handling...
    }

    for (;;) {
    }
}
