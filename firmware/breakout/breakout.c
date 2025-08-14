/**
 * @file breakout.c
 * @author Francisco Da Silva (francisco.dasilva@gwf.ch)
 *
 * @copyright Copyright (c) 2025 GWF AG
 *
 */

#include "breakout.h"

#include <stdio.h>
#include <stdbool.h>

#include "app.h"
#include "max7219.h"
#include "ssd1306.h"

#define PADDLE_WIDTH        3
#define WALL_INITIAL_HEIGHT 4

void app_matrix_clean(bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT]);

typedef struct {
	uint8_t x, y;  // ball position
	int8_t dx, dy; // direction
} ball_t;

static uint8_t paddle = 2; // x-position of left side of paddle
static ball_t ball;
static bool wall[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT];// 8x8 matrix

static void lcd_start(void);
static void update_matrix(bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT],
                          bool wall[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT],
                          ball_t ball,
                          uint8_t paddle);
static void ball_init(ball_t* ball);
static void update_paddle(uint8_t* paddle);
static void update_ball(ball_t* ball);
static void wall_init(bool wall[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT]);
static void reflect_ball(ball_t* ball);
static void wall_collision_handling(bool wall[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT], ball_t* ball);
static void border_collision_handling(ball_t* ball);
static void paddle_collision_handling(uint8_t paddle, ball_t* ball);

void breakout(void)
{
	lcd_start();
    app_matrix_clean(matrix);

    wall_init(wall);

    ball_init(&ball);

    do {
		update_paddle(&paddle);

		update_ball(&ball);

		border_collision_handling(&ball);
		wall_collision_handling(wall, &ball);
		paddle_collision_handling(paddle, &ball);

		update_matrix(matrix, wall, ball, paddle);
		max7219_set_matrix(&max7219, matrix);
    } while(true);
}

static void border_collision_handling(ball_t* ball)
{

}

static void paddle_collision_handling(uint8_t paddle, ball_t* ball)
{
	if (ball->y != 7) {
		// no collision possible
		return;
	}

	// check for left side collision
	if (ball->x == paddle) {
		// collision on left side of paddle detected!
		ball->y -= 1;
		ball->x -= 1;
		ball->dy = -1;
		ball->dx = -1;
	}

	// check for middle side collision
	if (ball->x == paddle + 1) {
		// collision on middle side of paddle detected!
		ball->y -= 2;
		ball->dy = -1;
		ball->dx = 0;
	}

	// check for middle side collision
	if (ball->x == paddle + 2) {
		// collision on right side of paddle detected!
		ball->y -= 1;
		ball->x += 1;
		ball->dy = -1;
		ball->dx = 1;
	}
}

static void lcd_start(void)
{
    app_lcd_print_title();

    SSD1306_GotoXY(0, APP_LCD_ROW_GAME_NAME);
    SSD1306_Puts("Breakout", &Font_7x10, 1);
    SSD1306_UpdateScreen();
}

static void update_matrix(bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT],
                          bool wall[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT],
                          ball_t ball,
                          uint8_t paddle)
{
	// updated matrix
    app_matrix_clean(matrix);

	// Draw wall
	for (uint8_t x = 0; x < MAX7219_COLUMN_AMOUNT; x++) {
		for (uint8_t y = 0; y < WALL_INITIAL_HEIGHT; y++) {
			if (wall[x][y]) {
				matrix[x][y] = true;
			}
		}
	}

	// Draw paddle
	uint8_t paddle_row = MAX7219_ROW_AMOUNT - 1;

	for (uint8_t i = 0; i < PADDLE_WIDTH; i++) {
		uint8_t px = paddle + i;

		if (px < MAX7219_COLUMN_AMOUNT) {
			matrix[px][paddle_row] = true;
		}
	}

	// Draw ball
	if ((ball.x < MAX7219_COLUMN_AMOUNT) && (ball.y < MAX7219_ROW_AMOUNT)) {
		matrix[ball.x][ball.y] = true;
	}
}

static void wall_init(bool wall[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT])
{
	for (uint8_t row = 0; row < WALL_INITIAL_HEIGHT; row++) {
		for (uint8_t col = 0; col < MAX7219_COLUMN_AMOUNT; col++) {
			wall[col][row] = true;
		}
	}
}

static void update_ball(ball_t* ball)
{
	static uint16_t counter = 0;

	if (counter >= 30) {
		counter = 0;
		ball->y = ball->y + ball->dy;
		ball->x = ball->x + ball->dx;
	}

	++counter;
}

static void update_paddle(uint8_t* paddle)
{
	button_t button = app_get_user_input();

	// update paddle
	switch (button) {
		case BUTTON_LEFT:
			if (*paddle > 0) {
				*paddle = *paddle - 1;
			}
			break;

		case BUTTON_RIGHT:
			if (*paddle < MAX7219_COLUMN_AMOUNT - PADDLE_WIDTH) {
				*paddle = *paddle + 1;
			}
			break;

		case BUTTON_NONE:
		default:
			break;
	}
}

static void ball_init(ball_t* ball)
{
    ball->x = 3;
    ball->y = 6;
    ball->dx = 0;
    ball->dy = -1;
}

static void reflect_ball(ball_t* ball)
{
	// mirroring direction
	ball->dx = -ball->dx;
	ball->dy = -ball->dy;

	// revert last move and move in new direction
	ball->y = ball->y + 2 * ball->dy;
	ball->x = ball->x + 2 * ball->dx;
}

static void wall_collision_handling(bool wall[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT], ball_t* ball)
{
	if (!wall[ball->x][ball->y]) {
		// no collision detected
		return;
	}

	// collision detected

	// remove wall segment
	wall[ball->x][ball->y] = false;

	// reflect ball
	reflect_ball(ball);
}
