/**
 * @file max7219.h
 * @author Timon Burkard (timon.burkard@gwf.ch)
 *
 * @copyright Copyright (c) 2025 GWF AG
 *
 */

#ifndef MAX7219_H_
#define MAX7219_H_

#include "stm32f0xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

#define MAX7219_COLUMN_AMOUNT 8
#define MAX7219_ROW_AMOUNT    8

/**
 * @brief MAX7219 handle
 */
typedef struct {
    SPI_HandleTypeDef* spi;     // Pointer to SPI handle
    GPIO_TypeDef*      cs_port; // SPI CS GPIO Port
    uint16_t           cs_pin;  // SPI CS GPIO Pin
} max7219_t;

/**
 * @brief MAX7219 error codes
 */
typedef enum {
    MAX7219_OK = 0,
    MAX7219_ERROR,
    MAX7219_COM_ERROR,
    MAX7219_WRONG_ADDRESS,
} max7219_error_t;

/**
 * @brief MAX7219 register addresses
 */
typedef enum {
    MAX7219_ADR_NO_OP        = 0x00,
    MAX7219_ADR_DIGIT_0      = 0x01,
    MAX7219_ADR_DIGIT_1      = 0x02,
    MAX7219_ADR_DIGIT_2      = 0x03,
    MAX7219_ADR_DIGIT_3      = 0x04,
    MAX7219_ADR_DIGIT_4      = 0x05,
    MAX7219_ADR_DIGIT_5      = 0x06,
    MAX7219_ADR_DIGIT_6      = 0x07,
    MAX7219_ADR_DIGIT_7      = 0x08,
    MAX7219_ADR_DECODE_MODE  = 0x09,
    MAX7219_ADR_INTENSITY    = 0x0A,
    MAX7219_ADR_SCAN_LIMIT   = 0x0B,
    MAX7219_ADR_SHUTDOWN     = 0x0C,
    MAX7219_ADR_DISPLAY_TEST = 0x0F,
    MAX7219_ADR_AMOUNT // Amount of registers (keep at end!)
} max7219_adr_t;

/**
 * @brief MAX7219 values for DECODE_MODE register
 */
typedef enum {
    MAX7219_REG_DECODE_MODE_NO_DECODE      = 0x00,
    MAX7219_REG_DECODE_MODE_CODE_B_DIG_0   = 0x01,
    MAX7219_REG_DECODE_MODE_CODE_B_DIG_0_3 = 0x0F,
    MAX7219_REG_DECODE_MODE_CODE_B_DIG_0_7 = 0xFF,
} max7219_reg_decode_mode_t;

/**
 * @brief MAX7219 values for SHUTDOWN register
 */
typedef enum {
    MAX7219_REG_SHUTDOWN_MODE_SHUTDOWN = 0x00,
    MAX7219_REG_SHUTDOWN_MODE_NORMAL   = 0x01,
} max7219_reg_shutdown_t;

/**
 * @brief MAX7219 values for INTENSITY register
 */
typedef enum {
    MAX7219_REG_INTENSITY_1_32  = 0x00, // min. intensity
    MAX7219_REG_INTENSITY_3_32  = 0x01,
    MAX7219_REG_INTENSITY_5_32  = 0x02,
    MAX7219_REG_INTENSITY_7_32  = 0x03,
    MAX7219_REG_INTENSITY_9_32  = 0x04,
    MAX7219_REG_INTENSITY_11_32 = 0x05,
    MAX7219_REG_INTENSITY_13_32 = 0x06,
    MAX7219_REG_INTENSITY_15_32 = 0x07,
    MAX7219_REG_INTENSITY_17_32 = 0x08,
    MAX7219_REG_INTENSITY_19_32 = 0x09,
    MAX7219_REG_INTENSITY_21_32 = 0x0A,
    MAX7219_REG_INTENSITY_23_32 = 0x0B,
    MAX7219_REG_INTENSITY_25_32 = 0x0C,
    MAX7219_REG_INTENSITY_27_32 = 0x0D,
    MAX7219_REG_INTENSITY_29_32 = 0x0E,
    MAX7219_REG_INTENSITY_31_32 = 0x0F, // max. intensity
} max7219_reg_intensity_t;

/**
 * @brief MAX7219 values for SCAN_LIMIT register
 */
typedef enum {
    MAX7219_REG_SCAN_LIMIT_DIG_0   = 0x00,
    MAX7219_REG_SCAN_LIMIT_DIG_0_1 = 0x01,
    MAX7219_REG_SCAN_LIMIT_DIG_0_2 = 0x02,
    MAX7219_REG_SCAN_LIMIT_DIG_0_3 = 0x03,
    MAX7219_REG_SCAN_LIMIT_DIG_0_4 = 0x04,
    MAX7219_REG_SCAN_LIMIT_DIG_0_5 = 0x05,
    MAX7219_REG_SCAN_LIMIT_DIG_0_6 = 0x06,
    MAX7219_REG_SCAN_LIMIT_DIG_0_7 = 0x07,
} max7219_reg_scan_limit_t;

/**
 * @brief MAX7219 values for DISPLAY_TEST register
 */
typedef enum {
    MAX7219_REG_DISPLAY_TEST_MODE_NORMAL = 0x00,
    MAX7219_REG_DISPLAY_TEST_MODE_TEST   = 0x01,
} max7219_reg_display_test_t;

typedef enum {
    MAX7219_SEGMENT_A  = (1 << 6),
    MAX7219_SEGMENT_B  = (1 << 5),
    MAX7219_SEGMENT_C  = (1 << 4),
    MAX7219_SEGMENT_D  = (1 << 3),
    MAX7219_SEGMENT_E  = (1 << 2),
    MAX7219_SEGMENT_F  = (1 << 1),
    MAX7219_SEGMENT_G  = (1 << 0),
    MAX7219_SEGMENT_DP = (1 << 7),
} max7219_segments_t;

typedef enum {
    MAX7219_COLUMN_0 = MAX7219_ADR_DIGIT_0,
    MAX7219_COLUMN_1 = MAX7219_ADR_DIGIT_1,
    MAX7219_COLUMN_2 = MAX7219_ADR_DIGIT_2,
    MAX7219_COLUMN_3 = MAX7219_ADR_DIGIT_3,
    MAX7219_COLUMN_4 = MAX7219_ADR_DIGIT_4,
    MAX7219_COLUMN_5 = MAX7219_ADR_DIGIT_5,
    MAX7219_COLUMN_6 = MAX7219_ADR_DIGIT_6,
    MAX7219_COLUMN_7 = MAX7219_ADR_DIGIT_7,
} max7219_columns_t;

/**
 * @brief MAX7219 initialization
 *
 * @param[out] max7219 -- Pointer to MAX7219 handle
 * @param[in] spi      -- Pointer to SPI handle to be used
 * @param[in] cs_port  -- SPI CS port to be used
 * @param[in] cs_pin   -- SPI CS pin to be used
 *
 * @return max7219_error_t --Error code
 */
max7219_error_t max7219_init(max7219_t* max7219, const SPI_HandleTypeDef* spi, const GPIO_TypeDef* cs_port, uint16_t cs_pin);

/**
 * @brief MAX7219 send data over SPI
 *
 * @param[in] max7219 -- Pointer to MAX7219 handle
 * @param[in] address -- Address to be written to
 * @param[in] data    -- Data to be written
 *
 * @return max7219_error_t -- Error code
 */
max7219_error_t max7219_send(const max7219_t* max7219, max7219_adr_t address, uint8_t data);

max7219_error_t max7219_set_matrix(max7219_t* max7219, bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT]);

#endif /* MAX7219_H_ */
