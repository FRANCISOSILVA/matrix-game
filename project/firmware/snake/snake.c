/**
 * @file snake.c
 * @author Timon Burkard (timon.burkard@gwf.ch)
 *
 * @copyright Copyright (c) 2025 GWF AG
 *
 */

#include "snake.h"

#include <stdio.h>

#include "app.h"
#include "max7219.h"
#include "ssd1306.h"

#define NO_FOOD 0xFF

typedef struct {
    uint8_t col;
    uint8_t row;
} coordinates_t;

typedef struct snake_part {
    uint8_t            col;
    uint8_t            row;
    struct snake_part* next;
} snake_part_t;

static snake_part_t* head                                             = NULL;
static snake_part_t  snake[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT] = { 0 };

static snake_part_t food = { NO_FOOD, NO_FOOD };

static void    add_head(snake_part_t** snake_head, coordinates_t new_head);
static void    add_head_remove_tail(snake_part_t** snake_head, coordinates_t new_head);
static bool    is_game_over(coordinates_t new_head);
static bool    is_eating(coordinates_t new_head);
static move_t  apply_new_head(snake_part_t** snake_head, coordinates_t new_head);
static move_t  move_left(snake_part_t** snake_head);
static move_t  move_right(snake_part_t** snake_head);
static move_t  move_up(snake_part_t** snake_head);
static move_t  move_down(snake_part_t** snake_head);
static uint8_t calc_score(void);
static void    print_score(uint16_t score);

void snake_init(void)
{
    for (uint8_t i = 0; i < MAX7219_COLUMN_AMOUNT; i++) {
        for (uint8_t j = 0; j < MAX7219_ROW_AMOUNT; j++) {
            snake[i][j].col  = i;
            snake[i][j].row  = j;
            snake[i][j].next = NULL;
        }
    }
}

void snake_lcd_start(void)
{
    char highscore[20] = "";

    sprintf(highscore, "Highscore: %d", app_flash_load_highscore());

    SSD1306_Clear();
    SSD1306_GotoXY(0, 0);
    SSD1306_Puts("GWF Schnupperlehre", &Font_7x10, 1);
    SSD1306_GotoXY(0, 10);
    SSD1306_Puts("-----------------", &Font_7x10, 1);
    SSD1306_GotoXY(0, 25);
    SSD1306_Puts("Snake Game", &Font_7x10, 1);
    SSD1306_GotoXY(0, 39);
    SSD1306_Puts(highscore, &Font_7x10, 1);
    SSD1306_UpdateScreen();
}

void snake_convert_to_matrix(bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT])
{
    snake_part_t* temp = head;

    app_matrix_clean(matrix);

    while (temp != NULL) {
        matrix[temp->col][temp->row] = true;
        temp                         = temp->next;
    }

    if (food.col != NO_FOOD) {
        matrix[food.col][food.row] = true;
    }
}

void snake_food_generate(void)
{
    do {
        food.col = rand() % MAX7219_COLUMN_AMOUNT;
        food.row = rand() % MAX7219_ROW_AMOUNT;
    } while (matrix[food.col][food.row] == true);
}

/**
 * @brief Snake move
 *
 * @param direction
 *
 * @return move_t
 */
move_t snake_move(button_t direction)
{
    switch (direction) {
    case BUTTON_UP:
        return move_up(&head);

    case BUTTON_DOWN:
        return move_down(&head);

    case BUTTON_LEFT:
        return move_left(&head);

    case BUTTON_RIGHT:
        return move_right(&head);

    default:
        return MOVE_GAME_OVER; // Not reachable
    }
}

void snake_handle_score(void)
{
    uint16_t score = calc_score();
    print_score(score);

    uint16_t highscore = app_flash_load_highscore();

    if (score > highscore) {
        app_flash_save_highscore(score);
    }
}

void snake_start_game(button_t* direction)
{
    // Start at the middle
    head       = &snake[MAX7219_COLUMN_AMOUNT / 2][MAX7219_ROW_AMOUNT / 2];
    head->next = NULL;

    food.col = NO_FOOD;
    food.row = NO_FOOD;

    snake_convert_to_matrix(matrix);

    if (max7219_set_matrix(&max7219, matrix) != MAX7219_OK) {
        for (;;) {
        } // Error handling...
    }

    *direction = BUTTON_RIGHT;
}

static void add_head(snake_part_t** snake_head, coordinates_t new_head)
{
    snake_part_t* new_head_node = &snake[new_head.col][new_head.row];

    new_head_node->next = *snake_head;

    *snake_head = new_head_node;
}

static void add_head_remove_tail(snake_part_t** snake_head, coordinates_t new_head)
{
    snake_part_t* temp;
    snake_part_t* new_head_node = &snake[new_head.col][new_head.row];

    new_head_node->next = *snake_head;

    if ((*snake_head)->next == NULL) {
        // The snake had only one part
        new_head_node->next = NULL;
        *snake_head         = new_head_node;
        return;
    }

    temp = *snake_head;

    while (temp->next->next != NULL) {
        temp = temp->next;
    }

    // Remove last part
    temp->next = NULL;

    *snake_head = new_head_node;
}

/**
 * @brief Check if the game is over
 *
 * @param new_head
 *
 * @return true  -- The game is over
 * @return false -- The game is not over
 */
static bool is_game_over(coordinates_t new_head)
{
    if ((new_head.col >= MAX7219_COLUMN_AMOUNT) || (new_head.row >= MAX7219_ROW_AMOUNT)) {
        return true;
    }

    snake_part_t* temp = head;

    while (temp != NULL) {
        if ((temp->col == new_head.col) && (temp->row == new_head.row)) {
            return true;
        }

        temp = temp->next;
    }

    return false;
}

/**
 * @brief Check if the snake is eating food
 *
 * @param new_head
 *
 * @return true  -- The snake is eating food
 * @return false -- The snake is not eating food
 */
static bool is_eating(coordinates_t new_head)
{
    return (new_head.col == food.col) && (new_head.row == food.row);
}

/**
 * @brief Apply the new head and do checks for eating food and game over
 *
 * @param snake_head
 * @param new_head
 *
 * @return move_t
 */
static move_t apply_new_head(snake_part_t** snake_head, coordinates_t new_head)
{
    if (is_game_over(new_head)) {
        return MOVE_GAME_OVER;
    }

    if (is_eating(new_head)) {
        // Eating food
        add_head(snake_head, new_head);
        return MOVE_EAT;
    }

    add_head_remove_tail(snake_head, new_head);

    return MOVE_NORMAL;
}

/**
 * @brief Move snake left
 *
 * @param snake_head
 *
 * @return move_t
 */
static move_t move_left(snake_part_t** snake_head)
{
    coordinates_t new_head;

    new_head.col = (*snake_head)->col - 1;
    new_head.row = (*snake_head)->row;

    return apply_new_head(snake_head, new_head);
}

/**
 * @brief Move snake right
 *
 * @param snake_head
 *
 * @return move_t
 */
static move_t move_right(snake_part_t** snake_head)
{
    coordinates_t new_head;

    new_head.col = (*snake_head)->col + 1;
    new_head.row = (*snake_head)->row;

    return apply_new_head(snake_head, new_head);
}

/**
 * @brief Move snake up
 *
 * @param snake_head
 *
 * @return move_t
 */
static move_t move_up(snake_part_t** snake_head)
{
    coordinates_t new_head;

    new_head.col = (*snake_head)->col;
    new_head.row = (*snake_head)->row - 1;

    return apply_new_head(snake_head, new_head);
}

/**
 * @brief Move snake down
 *
 * @param snake_head
 *
 * @return move_t
 */
static move_t move_down(snake_part_t** snake_head)
{
    coordinates_t new_head;

    new_head.col = (*snake_head)->col;
    new_head.row = (*snake_head)->row + 1;

    return apply_new_head(snake_head, new_head);
}

static uint8_t calc_score(void)
{
    uint8_t       score = 0;
    snake_part_t* temp  = head;

    while (temp != NULL) {
        score++;
        temp = temp->next;
    }

    return score;
}

static void print_score(uint16_t score)
{
    char string[20] = "";

    sprintf(string, "Score: %d", score);

    SSD1306_GotoXY(0, 53);
    SSD1306_Puts(string, &Font_7x10, 1);
    SSD1306_UpdateScreen();
}
