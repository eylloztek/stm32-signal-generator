/*
 * menu.h
 *
 *  Created on: Jun 14, 2026
 *      Author: Eylül Öztek
 */

#ifndef INC_MENU_H_
#define INC_MENU_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "ssd1306_conf.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "stdio.h"
#include "pwm.h"

#define MAIN_MENU_ITEM_COUNT        4U
#define MENU_BUTTON_DEBOUNCE_MS     50U

/*
 * Button active states
 *
 * If your buttons are connected with pull-up resistors:
 * pressed  = GPIO_PIN_RESET
 *
 * If your buttons are connected with pull-down resistors:
 * pressed  = GPIO_PIN_SET
 */
#define MENU_BUTTON_ACTIVE_STATE    GPIO_PIN_RESET
#define SELECT_ACTIVE_STATE         GPIO_PIN_RESET
#define ONOFF_ACTIVE_STATE          GPIO_PIN_RESET

void printMenuItems(uint8_t menuCount);
void handleMenuNavigation(void);
void setWaveform(void);
void setFrequency(void);
void setDutyCycle(void);
void showAbout(void);
void showInfo(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_MENU_H_ */
