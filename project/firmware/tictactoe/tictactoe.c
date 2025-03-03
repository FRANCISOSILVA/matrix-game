/**
 * @file tictactoe.c
 * @author Francisco Da Silva (francisco.dasilva@gwf.ch)
 *
 * @copyright Copyright (c) 2025 GWF AG
 *
 */

#include "tictactoe.h"

#include <stdio.h>

#include "app.h"
#include "max7219.h"
#include "ssd1306.h"

typedef struct {
    int row;
    int col;
} cursor_t;

typedef enum {
    NONE,
    X,
    O,
    DRAW
} field_t;

// [COL][ROW]
static field_t gamefield[3][3] = { NONE };

static void    print_cursor(cursor_t cursor, field_t active_player);
static void    show_grid(bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT]);
static void    lcd_start(void);
static void    convert_to_matrix(bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT]);
static void    start_game(void);
static void    player_move(field_t active_player);
static field_t check_winner(void);
static void    print_winner(field_t winner);

void tictactoe(void)
{
    field_t winner        = NONE;
    field_t active_player = X;

    show_grid(matrix);
    max7219_set_matrix(&max7219, matrix);
    lcd_start();

    while (app_get_user_input() == BUTTON_NONE) {
        // Wait for user to start the game
    }

    start_game();
    convert_to_matrix(matrix);
    max7219_set_matrix(&max7219, matrix);

    do {
        player_move(active_player);

        // Switch player
        if (active_player == X) {
            active_player = O;
        } else {
            active_player = X;
        }

        winner = check_winner();
    } while (winner == NONE);

    print_winner(winner);

    app_beep(BEEP_LONG_MS);

    while (app_get_user_input() == BUTTON_NONE) {
        // Wait for user to start the game
    }
}

static void clear_gamefield()
{
    for (uint8_t col = 0; col < 3; col++) {
        for (uint8_t row = 0; row < 3; row++) {
            gamefield[col][row] = NONE;
        }
    }
}

static void lcd_start(void)
{
    app_lcd_print_title();

    SSD1306_GotoXY(0, APP_LCD_ROW_GAME_NAME);
    SSD1306_Puts("TicTacToe", &Font_7x10, 1);
    SSD1306_UpdateScreen();
}

static void start_game(void)
{
    clear_gamefield();
}

static void show_grid(bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT])
{
    app_matrix_clean(matrix);

    // Set columns 2 and 5
    for (int row = 0; row < MAX7219_ROW_AMOUNT; ++row) {
        matrix[2][row] = true;
        matrix[5][row] = true;
    }

    // Set rows 2 and 5
    for (int col = 0; col < MAX7219_COLUMN_AMOUNT; ++col) {
        matrix[col][2] = true;
        matrix[col][5] = true;
    }
}

static void convert_to_matrix(bool matrix[MAX7219_COLUMN_AMOUNT][MAX7219_ROW_AMOUNT])
{
    app_matrix_clean(matrix);

    // [COL][ROW]
    if (gamefield[0][0] == O) {
        matrix[0][0] = true;
        matrix[0][1] = true;
        matrix[1][0] = true;
        matrix[1][1] = true;
    } else if (gamefield[0][0] == X) {
        matrix[0][0] = true;
        matrix[1][1] = true;
    }

    if (gamefield[1][0] == O) {
        matrix[3][0] = true;
        matrix[4][0] = true;
        matrix[3][1] = true;
        matrix[4][1] = true;
    } else if (gamefield[1][0] == X) {
        matrix[3][0] = true;
        matrix[4][1] = true;
    }

    if (gamefield[2][0] == O) {
        matrix[6][0] = true;
        matrix[7][0] = true;
        matrix[6][1] = true;
        matrix[7][1] = true;
    } else if (gamefield[2][0] == X) {
        matrix[6][0] = true;
        matrix[7][1] = true;
    }

    if (gamefield[0][1] == O) {
        matrix[0][3] = true;
        matrix[0][4] = true;
        matrix[1][3] = true;
        matrix[1][4] = true;
    } else if (gamefield[0][1] == X) {
        matrix[0][3] = true;
        matrix[1][4] = true;
    }

    if (gamefield[1][1] == O) {
        matrix[3][3] = true;
        matrix[3][4] = true;
        matrix[4][3] = true;
        matrix[4][4] = true;
    } else if (gamefield[1][1] == X) {
        matrix[3][3] = true;
        matrix[4][4] = true;
    }

    if (gamefield[2][1] == O) {
        matrix[6][3] = true;
        matrix[6][4] = true;
        matrix[7][3] = true;
        matrix[7][4] = true;
    } else if (gamefield[2][1] == X) {
        matrix[6][3] = true;
        matrix[7][4] = true;
    }

    if (gamefield[0][2] == O) {
        matrix[0][6] = true;
        matrix[0][7] = true;
        matrix[1][6] = true;
        matrix[1][7] = true;
    } else if (gamefield[0][2] == X) {
        matrix[0][6] = true;
        matrix[1][7] = true;
    }

    if (gamefield[1][2] == O) {
        matrix[3][6] = true;
        matrix[3][7] = true;
        matrix[4][6] = true;
        matrix[4][7] = true;
    } else if (gamefield[1][2] == X) {
        matrix[3][6] = true;
        matrix[4][7] = true;
    }

    if (gamefield[2][2] == O) {
        matrix[6][6] = true;
        matrix[6][7] = true;
        matrix[7][6] = true;
        matrix[7][7] = true;
    } else if (gamefield[2][2] == X) {
        matrix[6][6] = true;
        matrix[7][7] = true;
    }
}

static void print_cursor(cursor_t cursor, field_t active_player)
{
    convert_to_matrix(matrix);

    matrix[cursor.col * 3][cursor.row * 3]         = true;
    matrix[cursor.col * 3 + 1][cursor.row * 3 + 1] = true;

    if (active_player == X) {
        matrix[cursor.col * 3][cursor.row * 3 + 1] = false;
        matrix[cursor.col * 3 + 1][cursor.row * 3] = false;
    } else {
        matrix[cursor.col * 3][cursor.row * 3 + 1] = true;
        matrix[cursor.col * 3 + 1][cursor.row * 3] = true;
    }

    max7219_set_matrix(&max7219, matrix);
}

// Analyze gamefield to check if/who is the winner
static field_t check_winner(void)
{
    // Check columns
    for (uint8_t i = 0; i < 3; i++) {
        // Check column i
        if ((gamefield[i][0] == gamefield[i][1]) && (gamefield[i][1] == gamefield[i][2])) {
            // all fields in column i have the same value
            if (gamefield[i][0] != NONE) {
                return gamefield[i][0];
            }
        }
    }

    // Check rows
    for (uint8_t i = 0; i < 3; i++) {
        // Check row i
        if ((gamefield[0][i] == gamefield[1][i]) && (gamefield[1][i] == gamefield[2][i])) {
            // all fields in row i have the same value
            if (gamefield[0][i] != NONE) {
                return gamefield[0][i];
            }
        }
    }

    // Check diagonal which goes from top left to bottom right
    if ((gamefield[0][0] == gamefield[1][1]) && (gamefield[1][1] == gamefield[2][2])) {
        // all fields in this diagonal have the same value
        if (gamefield[0][0] != NONE) {
            return gamefield[0][0];
        }
    }

    // Check diagonal which goes from top right to bottom left
    if ((gamefield[2][0] == gamefield[1][1]) && (gamefield[1][1] == gamefield[0][2])) {
        // all fields in this diagonal have the same value
        if (gamefield[2][0] != NONE) {
            return gamefield[2][0];
        }
    }

    // Check if game needs to continue
    for (uint8_t col = 0; col < 3; col++) {
        for (uint8_t row = 0; row < 3; row++) {
            if (gamefield[col][row] == NONE) {
                return NONE;
            }
        }
    }

    return DRAW;
}

static void print_winner(field_t winner)
{
    char string[20] = "";

    switch (winner) {
    case O:
        sprintf(string, "Winner: O");
        break;
    case X:
        sprintf(string, "Winner: X");
        break;
    case DRAW:
        sprintf(string, "Winner: DRAW");
        break;

    default:
        break; // Not reachable
    }

    SSD1306_GotoXY(0, APP_LCD_ROW_GAME_DYNAMIC_0);
    SSD1306_Puts(string, &Font_7x10, 1);
    SSD1306_UpdateScreen();
}

static void player_move(field_t active_player)
{
    button_t button = BUTTON_NONE;

    // init curser
    cursor_t cursor = {
        .row = 0,
        .col = 0,
    };

    // find first empty field
    while (gamefield[cursor.col][cursor.row] != NONE) {
        ++cursor.col;

        if (cursor.col >= 3) {
            cursor.col = 0;
            ++cursor.row;

            if (cursor.row >= 3) {
                break;
            }
        }
    }

    bool already_occupied = false;

    // Let user decide which field he wants to choose
    do {
        button = app_get_user_input();

        switch (button) {
        case BUTTON_LEFT:
            if (cursor.col == 0) {
                break;
            }

            if (gamefield[cursor.col - 1][cursor.row] != NONE) {
                already_occupied = true;
            } else {
                already_occupied = false;
            }

            --cursor.col;

            break;

        case BUTTON_RIGHT:
            if (cursor.col == 2) {
                break;
            }

            if (gamefield[cursor.col + 1][cursor.row] != NONE) {
                already_occupied = true;
            } else {
                already_occupied = false;
            }

            ++cursor.col;

            break;

        case BUTTON_UP:
            if (cursor.row == 0) {
                break;
            }

            if (gamefield[cursor.col][cursor.row - 1] != NONE) {
                already_occupied = true;
            } else {
                already_occupied = false;
            }

            --cursor.row;

            break;

        case BUTTON_DOWN:
            if (cursor.row == 2) {
                break;
            }

            if (gamefield[cursor.col][cursor.row + 1] != NONE) {
                already_occupied = true;
            } else {
                already_occupied = false;
            }

            ++cursor.row;

            break;

        case BUTTON_CENTER:
        case BUTTON_NONE:
            break; // Nothing to do
        }

        print_cursor(cursor, active_player);

    } while ((button != BUTTON_CENTER) || (already_occupied));

    // Insert user choice into gamefield
    gamefield[cursor.col][cursor.row] = active_player;

    // Print
    convert_to_matrix(matrix);
    max7219_set_matrix(&max7219, matrix);
}
