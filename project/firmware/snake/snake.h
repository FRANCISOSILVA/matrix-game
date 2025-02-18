/**
 * @file snake.h
 * @author Timon Burkard (timon.burkard@gwf.ch)
 *
 * @copyright Copyright (c) 2025 GWF AG
 *
 */

#ifndef SNAKE_H_
#define SNAKE_H_

#include <stdbool.h>
#include <stdint.h>

#include "app.h"
#include "max7219.h"

#define SNAKE_SEQUENCE_PERIOD_10MS 35 // Period for each sequence (i.e., snake "steps") in multiple of 10ms

typedef enum {
    MOVE_NORMAL,    // A regular move (game not over; snake did not eat food)
    MOVE_EAT,       // Snake ate food
    MOVE_GAME_OVER, // Snake hit the wall or itself
} move_t;

void   snake_convert_to_matrix(bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT]);
void   snake_init(void);
void   snake_lcd_start(void);
void   snake_food_generate(void);
void   snake_handle_score(void);
move_t snake_move(button_t direction);
void   snake_start_game(button_t* direction);

#endif /* SNAKE_H_ */
