/**
 * @file max7219.c
 * @author Timon Burkard (timon.burkard@gwf.ch)
 *
 * @copyright Copyright (c) 2025 GWF AG
 *
 */

#include "max7219.h"

max7219_error_t max7219_init(max7219_t* max7219, const SPI_HandleTypeDef* spi, const GPIO_TypeDef* cs_port, uint16_t cs_pin)
{
    if (max7219 == NULL) {
        return MAX7219_ERROR;
    }

    if (spi == NULL) {
        return MAX7219_ERROR;
    }

    if (cs_port == NULL) {
        return MAX7219_ERROR;
    }

    max7219->spi     = (SPI_HandleTypeDef*)spi;
    max7219->cs_port = (GPIO_TypeDef*)cs_port;
    max7219->cs_pin  = cs_pin;

    return MAX7219_OK;
}

max7219_error_t max7219_send(const max7219_t* max7219, max7219_adr_t address, uint8_t data)
{
    uint16_t word;

    if (max7219 == NULL) {
        return MAX7219_ERROR;
    }

    if (address >= MAX7219_ADR_AMOUNT) {
        return MAX7219_WRONG_ADDRESS;
    }

    HAL_GPIO_WritePin(max7219->cs_port, max7219->cs_pin, GPIO_PIN_RESET);

    word = (address << 8) | data;

    if (HAL_SPI_Transmit(max7219->spi, (uint8_t*)&word, 1, HAL_MAX_DELAY) != HAL_OK) {
        return MAX7219_COM_ERROR;
    }

    HAL_GPIO_WritePin(max7219->cs_port, max7219->cs_pin, GPIO_PIN_SET);

    return MAX7219_OK;
}

static const uint8_t ROW_TO_SEGMENT[8] = {
    [0] = MAX7219_SEGMENT_A,
    [1] = MAX7219_SEGMENT_B,
    [2] = MAX7219_SEGMENT_C,
    [3] = MAX7219_SEGMENT_D,
    [4] = MAX7219_SEGMENT_E,
    [5] = MAX7219_SEGMENT_F,
    [6] = MAX7219_SEGMENT_G,
    [7] = MAX7219_SEGMENT_DP,
};

max7219_error_t max7219_set_matrix(max7219_t* max7219, bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT])
{
    if (max7219 == NULL) {
        return MAX7219_ERROR;
    }

    if (matrix == NULL) {
        return MAX7219_ERROR;
    }

    for (uint8_t column_idx = 0; column_idx < MAX7219_COLUMN_AMOUNT; column_idx++) {
        uint8_t row = 0;

        for (uint8_t row_idx = 0; row_idx < MAX7219_ROW_AMOUNT; row_idx++) {
            if (matrix[column_idx][row_idx] == true) {
                row |= ROW_TO_SEGMENT[row_idx];
            }
        }

        if (max7219_send(max7219, MAX7219_COLUMN_0 + column_idx, row) != MAX7219_OK) {
            return MAX7219_ERROR;
        }
    }

    return MAX7219_OK;
}
