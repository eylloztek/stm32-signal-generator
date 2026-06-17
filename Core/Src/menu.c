/*
 * menu.c
 *
 *  Created on: Jun 14, 2026
 *      Author: Eylül Öztek
 */

#include "menu.h"
#include "dac_waveform.h"

extern TIM_HandleTypeDef htim4;

static const char *main_menu_items[] = { "Set Waveform", "Set Frequency",
		"Set DutyCycle", "About" };

uint8_t selectedMenuItem = 0;
uint32_t frequency = 1000U;
uint32_t dutyCycle = 50U;
uint8_t onoffFlag = 1;

/*
 * Frequency settings
 */
#define FREQUENCY_MIN_HZ             1U

#define PWM_FREQUENCY_MAX_HZ         100000U
#define PWM_FREQUENCY_STEP_HZ        100U

#define SINE_FREQUENCY_MAX_HZ        10000U
#define SINE_FREQUENCY_STEP_HZ       10U

#define DAC_FREQUENCY_MAX_HZ         10000U
#define DAC_FREQUENCY_STEP_HZ        10U

#define DUTYCYCLE_MIN_PERCENT       0U
#define DUTYCYCLE_MAX_PERCENT       100U
#define DUTYCYCLE_STEP_PERCENT      5U

static uint8_t frequencyScreenNeedsUpdate = 1;
static uint8_t dutyCycleScreenNeedsUpdate = 1;
static uint8_t waveformScreenNeedsUpdate = 1;

#define ABOUT_PAGE_COUNT        2U

static uint8_t aboutPage = 0;
static uint8_t aboutScreenNeedsUpdate = 1;

typedef struct {
	GPIO_PinState lastRawState;
	GPIO_PinState stableState;
	uint32_t lastChangeTick;
	uint8_t initialized;
} ButtonState_t;

typedef enum {
	WAVEFORM_PWM_SQUARE = 0, WAVEFORM_DAC_SINE, WAVEFORM_DAC_TRIANGLE
} WaveformType_t;

static ButtonState_t menuButtonState = { 0 };
static ButtonState_t selectButtonState = { 0 };
static ButtonState_t onoffButtonState = { 0 };
WaveformType_t selectedWaveform = WAVEFORM_PWM_SQUARE;

static uint8_t lastSelectedMenuItem = 255;

static const char* getWaveformName(void) {
	switch (selectedWaveform) {
	case WAVEFORM_PWM_SQUARE:
		return "Square PWM";

	case WAVEFORM_DAC_SINE:
		return "Sine DAC";

	case WAVEFORM_DAC_TRIANGLE:
		return "Triangle DAC";

	default:
		return "Unknown";
	}
}

static const char* getWaveformShortName(void) {
	switch (selectedWaveform) {
	case WAVEFORM_PWM_SQUARE:
		return "PWM";

	case WAVEFORM_DAC_SINE:
		return "SINE";

	case WAVEFORM_DAC_TRIANGLE:
		return "TRI";

	default:
		return "UNK";
	}
}

static uint32_t getMaxFrequencyForSelectedWaveform(void) {
	if (selectedWaveform == WAVEFORM_PWM_SQUARE) {
		return PWM_FREQUENCY_MAX_HZ;
	}

	return DAC_FREQUENCY_MAX_HZ;
}

static uint32_t getFrequencyStepForSelectedWaveform(void) {
	if (selectedWaveform == WAVEFORM_PWM_SQUARE) {
		return PWM_FREQUENCY_STEP_HZ;
	}

	return DAC_FREQUENCY_STEP_HZ;
}

static void clampFrequencyForSelectedWaveform(void) {
	uint32_t maxFrequency = getMaxFrequencyForSelectedWaveform();

	if (frequency > maxFrequency) {
		frequency = maxFrequency;
		frequencyScreenNeedsUpdate = 1;
	}
}
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
	char buffer[24];

	ssd1306_Fill(Black);

	for (uint8_t i = 0; i < menuCount; i++) {
		ssd1306_SetCursor(0, i * 12);

		if (i == selectedMenuItem) {
			ssd1306_WriteString("> ", Font_7x10, White);
		} else {
			ssd1306_WriteString("  ", Font_7x10, White);
		}

		ssd1306_WriteString((char*) main_menu_items[i], Font_7x10, White);
	}

	ssd1306_SetCursor(0, 52);
	snprintf(buffer, sizeof(buffer), "%s %luHz", getWaveformShortName(),
			(unsigned long) frequency);
	ssd1306_WriteString(buffer, Font_7x10, White);

	ssd1306_UpdateScreen();
}

static void displayWaveformScreen(void) {
	ssd1306_Fill(Black);

	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("Set Waveform", Font_7x10, White);

	ssd1306_SetCursor(0, 18);
	ssd1306_WriteString((char*) getWaveformName(), Font_11x18, White);

	ssd1306_SetCursor(0, 44);
	ssd1306_WriteString("M/O:Change", Font_7x10, White);

	ssd1306_SetCursor(0, 54);
	ssd1306_WriteString("S:Back", Font_7x10, White);

	ssd1306_UpdateScreen();
}

/**
 * @brief Displays the frequency setting screen.
 */
static void displayFrequencyScreen(void) {
	char buffer[24];

	ssd1306_Fill(Black);

	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("Set Frequency", Font_7x10, White);

	ssd1306_SetCursor(0, 12);
	snprintf(buffer, sizeof(buffer), "Mode:%s", getWaveformShortName());
	ssd1306_WriteString(buffer, Font_7x10, White);

	ssd1306_SetCursor(0, 28);
	snprintf(buffer, sizeof(buffer), "%lu Hz", (unsigned long) frequency);
	ssd1306_WriteString(buffer, Font_11x18, White);

	ssd1306_SetCursor(0, 50);
	snprintf(buffer, sizeof(buffer), "Step:%lu S:Back",
			(unsigned long) getFrequencyStepForSelectedWaveform());
	ssd1306_WriteString(buffer, Font_7x10, White);

	ssd1306_UpdateScreen();
}

/**
 * @brief Displays the duty cycle setting screen.
 */
static void displayDutyCycleScreen(void) {
	char buffer[24];

	ssd1306_Fill(Black);

	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("Set DutyCycle", Font_7x10, White);

	ssd1306_SetCursor(0, 12);
	snprintf(buffer, sizeof(buffer), "Mode:%s", getWaveformShortName());
	ssd1306_WriteString(buffer, Font_7x10, White);

	ssd1306_SetCursor(0, 28);
	snprintf(buffer, sizeof(buffer), "%lu %%", (unsigned long) dutyCycle);
	ssd1306_WriteString(buffer, Font_11x18, White);

	ssd1306_SetCursor(0, 50);
	ssd1306_WriteString("M:+ O:- S:Back", Font_7x10, White);

	ssd1306_UpdateScreen();
}

static void showDutyCycleNotAvailable(void) {
	ssd1306_Fill(Black);

	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("Duty Cycle", Font_7x10, White);

	ssd1306_SetCursor(0, 16);
	ssd1306_WriteString("not used for", Font_7x10, White);

	ssd1306_SetCursor(0, 30);
	ssd1306_WriteString("Sine DAC mode", Font_7x10, White);

	ssd1306_SetCursor(0, 50);
	ssd1306_WriteString("S:Back", Font_7x10, White);

	ssd1306_UpdateScreen();
}

/**
 * @brief Toggles the PWM output on or off.
 */
static void toggleOutput(void) {
	if (onoffFlag == 0) {
		if (selectedWaveform == WAVEFORM_PWM_SQUARE) {
			pwmChannelStop(&htim4, TIM_CHANNEL_1);
		} else {
			DAC_Waveform_Stop();
		}

		onoffFlag = 1;
	} else {
		clampFrequencyForSelectedWaveform();

		if (selectedWaveform == WAVEFORM_PWM_SQUARE) {
			setPWMFreqDuty(frequency, dutyCycle);
			pwmChannelStart(&htim4, TIM_CHANNEL_1);
			onoffFlag = 0;
		} else if (selectedWaveform == WAVEFORM_DAC_SINE) {
			if (DAC_Waveform_StartSine(frequency) == DAC_WAVEFORM_OK) {
				onoffFlag = 0;
			} else {
				showInfo();
				HAL_Delay(700);
			}
		} else if (selectedWaveform == WAVEFORM_DAC_TRIANGLE) {
			if (DAC_Waveform_StartTriangle(frequency) == DAC_WAVEFORM_OK) {
				onoffFlag = 0;
			} else {
				showInfo();
				HAL_Delay(700);
			}
		}
	}
}

static void enterWaveformMenu(void) {
	waveformScreenNeedsUpdate = 1;

	while (1) {
		if (onoffFlag == 0) {
			showInfo();
			HAL_Delay(700);
			break;
		}

		setWaveform();

		if (Button_WasClicked(select_GPIO_Port,
		select_Pin,
		SELECT_ACTIVE_STATE, &selectButtonState)) {
			break;
		}
	}
}

/**
 * @brief Opens the frequency setting menu.
 */
static void enterFrequencyMenu(void) {
	frequencyScreenNeedsUpdate = 1;

	while (1) {
		if (onoffFlag == 0) {
			showInfo();
			HAL_Delay(700);
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
	dutyCycleScreenNeedsUpdate = 1;

	while (1) {
		if (onoffFlag == 0) {
			showInfo();
			HAL_Delay(700);
			break;
		}

		if (selectedWaveform != WAVEFORM_PWM_SQUARE) {
			ssd1306_Fill(Black);

			ssd1306_SetCursor(0, 0);
			ssd1306_WriteString("Duty Cycle", Font_7x10, White);

			ssd1306_SetCursor(0, 16);
			ssd1306_WriteString("only used in", Font_7x10, White);

			ssd1306_SetCursor(0, 30);
			ssd1306_WriteString("PWM mode", Font_7x10, White);

			ssd1306_SetCursor(0, 50);
			ssd1306_WriteString("S:Back", Font_7x10, White);

			ssd1306_UpdateScreen();

			if (Button_WasClicked(select_GPIO_Port,
			select_Pin,
			SELECT_ACTIVE_STATE, &selectButtonState)) {
				break;
			}

			continue;
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
	aboutPage = 0;
	aboutScreenNeedsUpdate = 1;

	while (1) {
		showAbout();

		if (Button_WasClicked(MENU_BUTTON_GPIO_Port,
		MENU_BUTTON_Pin,
		MENU_BUTTON_ACTIVE_STATE, &menuButtonState)) {
			aboutPage++;

			if (aboutPage >= ABOUT_PAGE_COUNT) {
				aboutPage = 0;
			}

			aboutScreenNeedsUpdate = 1;
		}

		if (Button_WasClicked(select_GPIO_Port,
		select_Pin,
		SELECT_ACTIVE_STATE, &selectButtonState)) {
			break;
		}
	}
}

/**
 * @brief Handles menu navigation and menu selection.
 *
 * The MENU button moves the selected item down by one step.
 * The SELECT button enters the selected menu item.
 * The ON/OFF button toggles the PWM output only in the main menu.
 */
void handleMenuNavigation(void) {
	if (lastSelectedMenuItem != selectedMenuItem) {
		printMenuItems(MAIN_MENU_ITEM_COUNT);
		lastSelectedMenuItem = selectedMenuItem;
	}

	if (Button_WasClicked(MENU_BUTTON_GPIO_Port,
	MENU_BUTTON_Pin,
	MENU_BUTTON_ACTIVE_STATE, &menuButtonState)) {
		selectedMenuItem++;

		if (selectedMenuItem >= MAIN_MENU_ITEM_COUNT) {
			selectedMenuItem = 0;
		}

		printMenuItems(MAIN_MENU_ITEM_COUNT);
		lastSelectedMenuItem = selectedMenuItem;
	}

	if (Button_WasClicked(ONOFF_BUTTON_GPIO_Port,
	ONOFF_BUTTON_Pin,
	ONOFF_ACTIVE_STATE, &onoffButtonState)) {
		toggleOutput();
	}

	if (Button_WasClicked(select_GPIO_Port,
	select_Pin,
	SELECT_ACTIVE_STATE, &selectButtonState)) {
		switch (selectedMenuItem) {
		case 0:
			enterWaveformMenu();
			break;

		case 1:
			enterFrequencyMenu();
			break;

		case 2:
			enterDutyCycleMenu();
			break;

		case 3:
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

void setWaveform(void) {
	if (Button_WasClicked(MENU_BUTTON_GPIO_Port,
	MENU_BUTTON_Pin,
	MENU_BUTTON_ACTIVE_STATE, &menuButtonState)
			|| Button_WasClicked(ONOFF_BUTTON_GPIO_Port,
			ONOFF_BUTTON_Pin,
			ONOFF_ACTIVE_STATE, &onoffButtonState)) {

		if (selectedWaveform == WAVEFORM_PWM_SQUARE) {
			selectedWaveform = WAVEFORM_DAC_SINE;
		} else if (selectedWaveform == WAVEFORM_DAC_SINE) {
			selectedWaveform = WAVEFORM_DAC_TRIANGLE;
		} else {
			selectedWaveform = WAVEFORM_PWM_SQUARE;
		}

		clampFrequencyForSelectedWaveform();

		waveformScreenNeedsUpdate = 1;
		frequencyScreenNeedsUpdate = 1;
		dutyCycleScreenNeedsUpdate = 1;
		aboutScreenNeedsUpdate = 1;
	}

	if (waveformScreenNeedsUpdate) {
		displayWaveformScreen();
		waveformScreenNeedsUpdate = 0;
	}
}

/**
 * @brief Changes the frequency value using buttons.
 *
 * MENU button increases the frequency.
 * ON/OFF button decreases the frequency.
 * SELECT button is handled by enterFrequencyMenu() and returns to the main menu.
 */
void setFrequency(void) {
	uint32_t maxFrequency = getMaxFrequencyForSelectedWaveform();
	uint32_t stepFrequency = getFrequencyStepForSelectedWaveform();

	clampFrequencyForSelectedWaveform();

	if (Button_WasClicked(MENU_BUTTON_GPIO_Port,
	MENU_BUTTON_Pin,
	MENU_BUTTON_ACTIVE_STATE, &menuButtonState)) {
		if (frequency <= (maxFrequency - stepFrequency)) {
			frequency += stepFrequency;
		} else {
			frequency = maxFrequency;
		}

		frequencyScreenNeedsUpdate = 1;
	}

	if (Button_WasClicked(ONOFF_BUTTON_GPIO_Port,
	ONOFF_BUTTON_Pin,
	ONOFF_ACTIVE_STATE, &onoffButtonState)) {
		if (frequency >= (FREQUENCY_MIN_HZ + stepFrequency)) {
			frequency -= stepFrequency;
		} else {
			frequency = FREQUENCY_MIN_HZ;
		}

		frequencyScreenNeedsUpdate = 1;
	}

	if (frequencyScreenNeedsUpdate) {
		displayFrequencyScreen();
		frequencyScreenNeedsUpdate = 0;
	}
}

/**
 * @brief Changes the duty cycle value using buttons.
 *
 * MENU button increases the duty cycle.
 * ON/OFF button decreases the duty cycle.
 * SELECT button is handled by enterDutyCycleMenu() and returns to the main menu.
 */
void setDutyCycle(void) {
	if (Button_WasClicked(MENU_BUTTON_GPIO_Port,
	MENU_BUTTON_Pin,
	MENU_BUTTON_ACTIVE_STATE, &menuButtonState)) {
		if (dutyCycle <= (DUTYCYCLE_MAX_PERCENT - DUTYCYCLE_STEP_PERCENT)) {
			dutyCycle += DUTYCYCLE_STEP_PERCENT;
		} else {
			dutyCycle = DUTYCYCLE_MAX_PERCENT;
		}

		dutyCycleScreenNeedsUpdate = 1;
	}

	if (Button_WasClicked(ONOFF_BUTTON_GPIO_Port,
	ONOFF_BUTTON_Pin,
	ONOFF_ACTIVE_STATE, &onoffButtonState)) {
		if (dutyCycle >= (DUTYCYCLE_MIN_PERCENT + DUTYCYCLE_STEP_PERCENT)) {
			dutyCycle -= DUTYCYCLE_STEP_PERCENT;
		} else {
			dutyCycle = DUTYCYCLE_MIN_PERCENT;
		}

		dutyCycleScreenNeedsUpdate = 1;
	}

	if (dutyCycleScreenNeedsUpdate) {
		displayDutyCycleScreen();
		dutyCycleScreenNeedsUpdate = 0;
	}
}

void showAbout(void) {
	char buffer[24];

	if (!aboutScreenNeedsUpdate) {
		return;
	}

	ssd1306_Fill(Black);

	if (aboutPage == 0) {
		ssd1306_SetCursor(0, 0);
		ssd1306_WriteString("Signal Generator", Font_7x10, White);

		ssd1306_SetCursor(0, 12);
		snprintf(buffer, sizeof(buffer), "Mode:%s", getWaveformShortName());
		ssd1306_WriteString(buffer, Font_7x10, White);

		ssd1306_SetCursor(0, 24);
		snprintf(buffer, sizeof(buffer), "Freq:%luHz",
				(unsigned long) frequency);
		ssd1306_WriteString(buffer, Font_7x10, White);

		ssd1306_SetCursor(0, 36);

		if (selectedWaveform == WAVEFORM_PWM_SQUARE) {
			snprintf(buffer, sizeof(buffer), "Duty:%lu%%",
					(unsigned long) dutyCycle);
		} else {
			snprintf(buffer, sizeof(buffer), "Duty:N/A");
		}

		ssd1306_WriteString(buffer, Font_7x10, White);

		ssd1306_SetCursor(0, 48);
		ssd1306_WriteString("STM32F446RE", Font_7x10, White);

		ssd1306_SetCursor(98, 48);
		ssd1306_WriteString("1/3", Font_7x10, White);
	} else if (aboutPage == 1) {
		ssd1306_SetCursor(0, 0);
		ssd1306_WriteString("Output Types", Font_7x10, White);

		ssd1306_SetCursor(0, 14);
		ssd1306_WriteString("PWM : TIM4 CH1", Font_7x10, White);

		ssd1306_SetCursor(0, 28);
		ssd1306_WriteString("SINE: DAC OUT1", Font_7x10, White);

		ssd1306_SetCursor(0, 42);
		ssd1306_WriteString("DAC trig: TIM2", Font_7x10, White);

		ssd1306_SetCursor(98, 48);
		ssd1306_WriteString("2/3", Font_7x10, White);
	} else {
		ssd1306_SetCursor(0, 0);
		ssd1306_WriteString("Controls", Font_7x10, White);

		ssd1306_SetCursor(0, 12);
		ssd1306_WriteString("MENU : Next/+", Font_7x10, White);

		ssd1306_SetCursor(0, 24);
		ssd1306_WriteString("ONOFF: Out/-", Font_7x10, White);

		ssd1306_SetCursor(0, 36);
		ssd1306_WriteString("SEL  : Enter/Back", Font_7x10, White);

		ssd1306_SetCursor(0, 48);
		ssd1306_WriteString("M:Next S:Back", Font_7x10, White);

		ssd1306_SetCursor(98, 48);
		ssd1306_WriteString("3/3", Font_7x10, White);
	}

	ssd1306_UpdateScreen();
	aboutScreenNeedsUpdate = 0;
}

void showInfo(void) {
	ssd1306_Fill(Black);

	ssd1306_SetCursor(0, 0);
	ssd1306_WriteString("Output is ON", Font_7x10, White);

	ssd1306_SetCursor(0, 17);
	ssd1306_WriteString("Stop output first", Font_7x10, White);

	ssd1306_UpdateScreen();
}
