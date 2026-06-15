/*
 * menu.c
 *
 *  Created on: Jun 14, 2026
 *      Author: Eylül Öztek
 */

#include "menu.h"

extern TIM_HandleTypeDef htim4;

static const char *main_menu_items[] = { "Set Frequency", "Set DutyCycle",
		"About" };

uint8_t selectedMenuItem = 0;
uint32_t frequency = 0;
uint32_t dutyCycle = 0;
uint8_t onoffFlag = 1;

typedef struct {
	GPIO_PinState lastRawState;
	GPIO_PinState stableState;
	uint32_t lastChangeTick;
	uint8_t initialized;
} ButtonState_t;

static ButtonState_t menuButtonState = { 0 };
static ButtonState_t selectButtonState = { 0 };
static ButtonState_t onoffButtonState = { 0 };

static uint8_t lastSelectedMenuItem = 255;

/**
 * @brief Checks whether a button press event occurred.
 *
 * This function uses edge detection and software debounce.
 * It returns 1 only once when the button changes to the pressed state.
 *
 * @param GPIOx GPIO port of the button.
 * @param GPIO_Pin GPIO pin of the button.
 * @param activeState Logic level that represents the pressed state.
 * @param buttonState Pointer to the button state structure.
 *
 * @return 1 if a valid button click is detected, otherwise 0.
 */
static uint8_t Button_WasClicked(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin,
		GPIO_PinState activeState, ButtonState_t *buttonState) {
	GPIO_PinState rawState = HAL_GPIO_ReadPin(GPIOx, GPIO_Pin);
	uint32_t now = HAL_GetTick();

	if (!buttonState->initialized) {
		buttonState->lastRawState = rawState;
		buttonState->stableState = rawState;
		buttonState->lastChangeTick = now;
		buttonState->initialized = 1;
		return 0;
	}

	if (rawState != buttonState->lastRawState) {
		buttonState->lastRawState = rawState;
		buttonState->lastChangeTick = now;
	}

	if ((now - buttonState->lastChangeTick) >= MENU_BUTTON_DEBOUNCE_MS) {
		if (rawState != buttonState->stableState) {
			buttonState->stableState = rawState;

			if (buttonState->stableState == activeState) {
				return 1;
			}
		}
	}

	return 0;
}

/**
 * @brief Displays the main menu items on the OLED screen.
 *
 * @param menuCount Number of menu items to display.
 */
void printMenuItems(uint8_t menuCount) {
	ssd1306_Fill(Black);

	for (uint8_t i = 0; i < menuCount; i++) {
		ssd1306_SetCursor(0, i * 20);

		if (i == selectedMenuItem) {
			ssd1306_WriteString("> ", Font_7x10, White);
		} else {
			ssd1306_WriteString("  ", Font_7x10, White);
		}

		ssd1306_WriteString((char*) main_menu_items[i], Font_7x10, White);
	}

	ssd1306_UpdateScreen();
}

/**
 * @brief Toggles the PWM output on or off.
 */
static void toggleOutput(void) {
	if (onoffFlag == 0) {
		pwmChannelStop(&htim4, TIM_CHANNEL_1);
		onoffFlag = 1;
	} else {
		setPWMFreqDuty(frequency, dutyCycle);
		pwmChannelStart(&htim4, TIM_CHANNEL_1);
		onoffFlag = 0;
	}
}

/**
 * @brief Opens the frequency setting menu.
 */
static void enterFrequencyMenu(void) {
	while (1) {
		if (onoffFlag == 0) {
			showInfo();
			break;
		}

		setFrequency();

		if (Button_WasClicked(select_GPIO_Port,
		select_Pin,
		SELECT_ACTIVE_STATE, &selectButtonState)) {
			break;
		}
	}
}

/**
 * @brief Opens the duty cycle setting menu.
 */
static void enterDutyCycleMenu(void) {
	while (1) {
		if (onoffFlag == 0) {
			showInfo();
			break;
		}

		setDutyCycle();

		if (Button_WasClicked(select_GPIO_Port,
		select_Pin,
		SELECT_ACTIVE_STATE, &selectButtonState)) {
			break;
		}
	}
}

/**
 * @brief Opens the about page.
 */
static void enterAboutMenu(void) {
	while (1) {
		showAbout();

		if (Button_WasClicked(select_GPIO_Port, select_Pin,
		SELECT_ACTIVE_STATE, &selectButtonState)) {
			break;
		}
	}
}

/**
 * @brief Handles menu navigation and menu selection.
 *
 * Rotary encoder based navigation was removed.
 * The MENU button moves the selected item down by one step.
 * The SELECT button enters the selected menu item.
 * The ON/OFF button toggles the PWM output.
 */
void handleMenuNavigation(void) {
	if (lastSelectedMenuItem != selectedMenuItem) {
		printMenuItems(MAIN_MENU_ITEM_COUNT);
		lastSelectedMenuItem = selectedMenuItem;
	}

	if (Button_WasClicked(MENU_BUTTON_GPIO_Port, MENU_BUTTON_Pin,
	MENU_BUTTON_ACTIVE_STATE, &menuButtonState)) {
		selectedMenuItem++;

		if (selectedMenuItem >= MAIN_MENU_ITEM_COUNT) {
			selectedMenuItem = 0;
		}

		printMenuItems(MAIN_MENU_ITEM_COUNT);
		lastSelectedMenuItem = selectedMenuItem;
	}

	if (Button_WasClicked(ONOFF_BUTTON_GPIO_Port, ONOFF_BUTTON_Pin,
	ONOFF_ACTIVE_STATE, &onoffButtonState)) {
		toggleOutput();
	}

	if (Button_WasClicked(select_GPIO_Port, select_Pin,
	SELECT_ACTIVE_STATE, &selectButtonState)) {
		switch (selectedMenuItem) {
		case 0:
			enterFrequencyMenu();
			break;

		case 1:
			enterDutyCycleMenu();
			break;

		case 2:
			enterAboutMenu();
			break;

		default:
			selectedMenuItem = 0;
			break;
		}

		printMenuItems(MAIN_MENU_ITEM_COUNT);
		lastSelectedMenuItem = selectedMenuItem;
	}
}

void setFrequency(void) {
	ssd1306_Fill(Black);
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("Set Frequency", Font_7x10, White);
	ssd1306_UpdateScreen();
}

void setDutyCycle(void) {
	ssd1306_Fill(Black);
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("Set DutyCycle", Font_7x10, White);
	ssd1306_UpdateScreen();
}

void showAbout(void) {
	ssd1306_Fill(Black);
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("Signal Generator", Font_7x10, White);
	ssd1306_SetCursor(0, 17);
	ssd1306_WriteString("STM32F446RE", Font_7x10, White);
	ssd1306_UpdateScreen();
}

void showInfo(void) {
	ssd1306_Fill(Black);
	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("Output is ON", Font_7x10, White);
	ssd1306_SetCursor(0, 17);
	ssd1306_WriteString("Stop output first", Font_7x10, White);
	ssd1306_UpdateScreen();
}
