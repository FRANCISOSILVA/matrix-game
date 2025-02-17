/**
 * @file app.c
 * @author Timon Burkard (timon.burkard@gwf.ch)
 *
 * @copyright Copyright (c) 2025 GWF AG
 *
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"
#include "max7219.h"
#include "ssd1306.h"

#define SEQUENCE_PERIOD_10MS 35 // Period for each sequence (i.e., snake "steps") in multiple of 10ms

#define BEEP_SHORT_MS 75
#define BEEP_LONG_MS  750

#define FLASH_HIGHSCORE_ADDRESS 0x08007F00 // Last page

typedef struct {
    uint8_t col;
    uint8_t row;
} coordinates_t;

typedef struct snake_part {
    uint8_t            col;
    uint8_t            row;
    struct snake_part* next;
} snake_part_t;

typedef enum button {
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_CENTER,
    BUTTON_NONE,
} button_t;

typedef enum {
    MOVE_NORMAL,    // A regular move (game not over; snake did not eat food)
    MOVE_EAT,       // Snake ate food
    MOVE_GAME_OVER, // Snake hit the wall or itself
} move_t;

extern SPI_HandleTypeDef hspi1;

static max7219_t max7219 = { 0 };

static bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT] = { false };

static snake_part_t* head                                             = NULL;
static snake_part_t  snake[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT] = { 0 };

static snake_part_t food = { 0 };

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

static uint16_t flash_load_highscore()
{
    return *((uint16_t*)FLASH_HIGHSCORE_ADDRESS); // Read stored value
}

/**
 * @brief Initialize the highscore in flash memory, if not yet done
 */
static void flash_init_highscore()
{
    if (flash_load_highscore() == 0xFFFF) {
        flash_save_highscore(0);
    }
}

static void snake_init(snake_part_t snake[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT])
{
    for (uint8_t i = 0; i < MAX7219_COLUMN_AMOUNT; i++) {
        for (uint8_t j = 0; j < MAX7219_ROW_AMOUNT; j++) {
            snake[i][j].col  = i;
            snake[i][j].row  = j;
            snake[i][j].next = NULL;
        }
    }
}

static void matrix_clean(bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT])
{
    for (uint8_t i = 0; i < MAX7219_COLUMN_AMOUNT; i++) {
        for (uint8_t j = 0; j < MAX7219_ROW_AMOUNT; j++) {
            matrix[i][j] = false;
        }
    }
}

static void convert_to_matrix(snake_part_t* snake, snake_part_t* food, bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT])
{
    snake_part_t* head = snake;

    matrix_clean(matrix);

    while (head != NULL) {
        matrix[head->col][head->row] = true;
        head                         = head->next;
    }

    if (food != NULL) {
        matrix[food->col][food->row] = true;
    }
}

static snake_part_t food_generate(void)
{
    snake_part_t food = { 0 };

    do {
        food.col = rand() % MAX7219_COLUMN_AMOUNT;
        food.row = rand() % MAX7219_ROW_AMOUNT;
    } while (matrix[food.col][food.row] == true);

    return food;
}

static button_t get_user_input(void)
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

    HAL_Delay(10); // 10 ms: Button debouncing

    return button;
}

static void snake_add_head(snake_part_t** snake_head, coordinates_t new_head)
{
    snake_part_t* new_head_node = &snake[new_head.col][new_head.row];

    new_head_node->next = *snake_head;

    *snake_head = new_head_node;
}

static void snake_add_head_remove_tail(snake_part_t** snake_head, coordinates_t new_head)
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
static move_t snake_apply_new_head(snake_part_t** snake_head, coordinates_t new_head)
{
    if (is_game_over(new_head)) {
        return MOVE_GAME_OVER;
    }

    if (is_eating(new_head)) {
        // Eating food
        snake_add_head(snake_head, new_head);
        return MOVE_EAT;
    }

    snake_add_head_remove_tail(snake_head, new_head);

    return MOVE_NORMAL;
}

/**
 * @brief Move snake left
 *
 * @param snake_head
 *
 * @return move_t
 */
static move_t snake_move_left(snake_part_t** snake_head)
{
    coordinates_t new_head;

    new_head.col = (*snake_head)->col - 1;
    new_head.row = (*snake_head)->row;

    return snake_apply_new_head(snake_head, new_head);
}

/**
 * @brief Move snake right
 *
 * @param snake_head
 *
 * @return move_t
 */
static move_t snake_move_right(snake_part_t** snake_head)
{
    coordinates_t new_head;

    new_head.col = (*snake_head)->col + 1;
    new_head.row = (*snake_head)->row;

    return snake_apply_new_head(snake_head, new_head);
}

/**
 * @brief Move snake up
 *
 * @param snake_head
 *
 * @return move_t
 */
static move_t snake_move_up(snake_part_t** snake_head)
{
    coordinates_t new_head;

    new_head.col = (*snake_head)->col;
    new_head.row = (*snake_head)->row - 1;

    return snake_apply_new_head(snake_head, new_head);
}

/**
 * @brief Move snake down
 *
 * @param snake_head
 *
 * @return move_t
 */
static move_t snake_move_down(snake_part_t** snake_head)
{
    coordinates_t new_head;

    new_head.col = (*snake_head)->col;
    new_head.row = (*snake_head)->row + 1;

    return snake_apply_new_head(snake_head, new_head);
}

/**
 * @brief Snake move
 *
 * @param direction
 *
 * @return move_t
 */
static move_t snake_move(button_t direction)
{
    switch (direction) {
    case BUTTON_UP:
        return snake_move_up(&head);

    case BUTTON_DOWN:
        return snake_move_down(&head);

    case BUTTON_LEFT:
        return snake_move_left(&head);

    case BUTTON_RIGHT:
        return snake_move_right(&head);

    default:
        return MOVE_GAME_OVER; // Not reachable
    }
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

static void handle_score(void)
{
    uint16_t score = calc_score();
    print_score(score);

    uint16_t highscore = flash_load_highscore();

    if (score > highscore) {
        flash_save_highscore(score);
    }
}

static void lcd_start(void)
{
    char highscore[20] = "";

    sprintf(highscore, "Highscore: %d", flash_load_highscore());

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

static void start_game(button_t* direction)
{
    // Start at the middle
    head       = &snake[MAX7219_COLUMN_AMOUNT / 2][MAX7219_ROW_AMOUNT / 2];
    head->next = NULL;

    convert_to_matrix(head, NULL, matrix);

    if (max7219_set_matrix(&max7219, matrix) != MAX7219_OK) {
        for (;;) {
        } // Error handling...
    }

    *direction = BUTTON_RIGHT;
}

static void beep(uint16_t duration_ms)
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

    flash_init_highscore();

    SSD1306_Init();
    lcd_start();

    snake_init(snake);

    for (;;) {
        while (get_user_input() == BUTTON_NONE) {
            // Wait for user to start the game
        }

        srand(HAL_GetTick());
        lcd_start();
        start_game(&direction);

        do {
            beep(BEEP_SHORT_MS);
            food = food_generate();

            do {
                for (uint8_t i = 0; i < SEQUENCE_PERIOD_10MS; i++) {
                    button = get_user_input(); // Takes 10 ms

                    if (button != BUTTON_NONE) {
                        direction = button;
                    }
                }

                move_state = snake_move(direction);

                convert_to_matrix(head, &food, matrix);

                if (max7219_set_matrix(&max7219, matrix) != MAX7219_OK) {
                    for (;;) {
                    } // Error handling...
                }
            } while (move_state == MOVE_NORMAL);
        } while (move_state != MOVE_GAME_OVER);

        beep(BEEP_LONG_MS);
        handle_score();
    }
}
