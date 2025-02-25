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

#define MAX_SNAKE_LENGTH 63

#define FLASH_HIGHSCORE_ADDRESS 0x08007F00 // Last page

#define SNAKE_SEQUENCE_PERIOD_10MS 25 // Period for each sequence (i.e., snake "steps") in multiple of 10ms

typedef enum {
    MOVE_NORMAL,    // A regular move (game not over; snake did not eat food)
    MOVE_EAT,       // Snake ate food
    MOVE_GAME_OVER, // Snake hit the wall or itself
} move_t;

typedef struct {
    uint8_t col;
    uint8_t row;
} coordinates_t;

typedef struct snake_part {
    uint8_t            col;
    uint8_t            row;
    struct snake_part* next;
} snake_part_t;

static snake_part_t* head                                                    = NULL;
static snake_part_t  snake_fields[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT] = { 0 };

static snake_part_t food = { NO_FOOD, NO_FOOD };

static void     add_head(snake_part_t** snake_head, coordinates_t new_head);
static void     add_head_remove_tail(snake_part_t** snake_head, coordinates_t new_head);
static bool     is_game_over(coordinates_t new_head);
static bool     is_eating(coordinates_t new_head);
static move_t   apply_new_head(snake_part_t** snake_head, coordinates_t new_head);
static move_t   move_left(snake_part_t** snake_head);
static move_t   move_right(snake_part_t** snake_head);
static move_t   move_up(snake_part_t** snake_head);
static move_t   move_down(snake_part_t** snake_head);
static uint8_t  calc_score(void);
static void     print_score(uint16_t score);
static void     flash_init_highscore(void);
static uint16_t flash_load_highscore(void);
static void     flash_save_highscore(uint16_t score);
static void     convert_to_matrix(bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT]);
static void     init(void);
static void     lcd_start(void);
static void     food_generate(void);
static void     handle_score(void);
static move_t   move_snake(button_t direction);
static void     start_game(button_t* direction);

void snake(void)
{
    button_t button     = BUTTON_NONE;
    button_t direction  = BUTTON_RIGHT;
    move_t   move_state = MOVE_NORMAL;

    srand(HAL_GetTick());

    flash_init_highscore();

    init();
    lcd_start();
    start_game(&direction);

    while (app_get_user_input() == BUTTON_NONE) {
        // Wait for user to start the game
    }

    do {
        app_beep(BEEP_SHORT_MS);
        food_generate();

        do {
            for (uint8_t i = 0; i < SNAKE_SEQUENCE_PERIOD_10MS; i++) {
                button = app_get_user_input(); // Takes 10 ms

                if ((button != BUTTON_NONE) && (button != BUTTON_CENTER)) {
                    direction = button;
                }
            }

            move_state = move_snake(direction);

            convert_to_matrix(matrix);

            if (max7219_set_matrix(&max7219, matrix) != MAX7219_OK) {
                for (;;) {
                } // Error handling...
            }
        } while (move_state == MOVE_NORMAL);
    } while (move_state != MOVE_GAME_OVER);

    app_beep(BEEP_LONG_MS);
    handle_score();

    while (app_get_user_input() == BUTTON_NONE) {
        // Wait for user to start the game
    }
}

static void init(void)
{
    for (uint8_t i = 0; i < MAX7219_COLUMN_AMOUNT; i++) {
        for (uint8_t j = 0; j < MAX7219_ROW_AMOUNT; j++) {
            snake_fields[i][j].col  = i;
            snake_fields[i][j].row  = j;
            snake_fields[i][j].next = NULL;
        }
    }
}

static void lcd_start(void)
{
    char highscore[20] = "";

    sprintf(highscore, "Highscore: %d", flash_load_highscore());

    app_lcd_print_title();

    SSD1306_GotoXY(0, APP_LCD_ROW_GAME_NAME);
    SSD1306_Puts("Snake Game", &Font_7x10, 1);
    SSD1306_GotoXY(0, APP_LCD_ROW_GAME_DYNAMIC_0);
    SSD1306_Puts(highscore, &Font_7x10, 1);
    SSD1306_UpdateScreen();
}

static void convert_to_matrix(bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT])
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

static void food_generate(void)
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
static move_t move_snake(button_t direction)
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

static void handle_score(void)
{
    uint16_t score = calc_score();
    print_score(score);

    uint16_t highscore = flash_load_highscore();

    if (score > highscore) {
        flash_save_highscore(score);
    }
}

static void start_game(button_t* direction)
{
    // Start at the middle
    head       = &snake_fields[MAX7219_COLUMN_AMOUNT / 2][MAX7219_ROW_AMOUNT / 2];
    head->next = NULL;

    food.col = NO_FOOD;
    food.row = NO_FOOD;

    convert_to_matrix(matrix);

    if (max7219_set_matrix(&max7219, matrix) != MAX7219_OK) {
        for (;;) {
        } // Error handling...
    }

    *direction = BUTTON_RIGHT;
}

static void add_head(snake_part_t** snake_head, coordinates_t new_head)
{
    snake_part_t* new_head_node = &snake_fields[new_head.col][new_head.row];

    new_head_node->next = *snake_head;

    *snake_head = new_head_node;
}

static void add_head_remove_tail(snake_part_t** snake_head, coordinates_t new_head)
{
    snake_part_t* temp;
    snake_part_t* new_head_node = &snake_fields[new_head.col][new_head.row];

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

    uint8_t       snake_length = 0;
    snake_part_t* temp         = head;

    while (temp != NULL) {
        if ((temp->col == new_head.col) && (temp->row == new_head.row)) {
            return true;
        }

        temp = temp->next;

        ++snake_length;
    }

    if (snake_length >= MAX_SNAKE_LENGTH) {
        return true;
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

    SSD1306_GotoXY(0, APP_LCD_ROW_GAME_DYNAMIC_1);
    SSD1306_Puts(string, &Font_7x10, 1);
    SSD1306_UpdateScreen();
}

/**
 * @brief Initialize the highscore in flash memory, if not yet done
 */
static void flash_init_highscore(void)
{
    if (flash_load_highscore() == 0xFFFF) {
        flash_save_highscore(0);
    }
}

static uint16_t flash_load_highscore(void)
{
    return *((uint16_t*)FLASH_HIGHSCORE_ADDRESS); // Read stored value
}

static void flash_save_highscore(uint16_t score)
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
