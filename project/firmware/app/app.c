/*
 * app.c
 *
 *  Created on: Feb 5, 2025
 *      Author: timon.burkard
 */

#include "ssd1306.h"

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

	for (;;) {

	}
}
