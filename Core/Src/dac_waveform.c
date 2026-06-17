/*
 * dac_waveform.c
 *
 *  Created on: Jun 17, 2026
 *      Author: Eylül Öztek
 */

#include "dac_waveform.h"
#include <math.h>

extern DAC_HandleTypeDef hdac;
extern TIM_HandleTypeDef htim2;

#define DAC_WAVEFORM_PI               3.14159265f
#define DAC_WAVEFORM_MAX_12BIT_VALUE  4095U
#define DAC_WAVEFORM_MID_SCALE        2048.0f
#define DAC_WAVEFORM_AMPLITUDE        2047.0f

static uint32_t sineTable[DAC_WAVEFORM_SAMPLE_COUNT];
static uint32_t triangleTable[DAC_WAVEFORM_SAMPLE_COUNT];

static uint8_t tablesGenerated = 0;

/**
 * @brief Returns the TIM2 input clock frequency.
 *
 * On STM32F4, if the APB1 prescaler is not 1,
 * timer clock becomes 2 x PCLK1.
 *
 * @return TIM2 clock frequency in Hz.
 */
static uint32_t DAC_Waveform_GetTIM2ClockHz(void) {
	uint32_t timerClockHz = HAL_RCC_GetPCLK1Freq();

	if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) {
		timerClockHz *= 2U;
	}

	return timerClockHz;
}

/**
 * @brief Generates a 12-bit sine lookup table.
 */
static void DAC_Waveform_GenerateSineTable(void) {
	for (uint32_t i = 0; i < DAC_WAVEFORM_SAMPLE_COUNT; i++) {
		float angle = (2.0f * DAC_WAVEFORM_PI * (float) i)
				/ (float) DAC_WAVEFORM_SAMPLE_COUNT;

		float sample = DAC_WAVEFORM_MID_SCALE
				+ (DAC_WAVEFORM_AMPLITUDE * sinf(angle));

		if (sample < 0.0f) {
			sample = 0.0f;
		}

		if (sample > (float) DAC_WAVEFORM_MAX_12BIT_VALUE) {
			sample = (float) DAC_WAVEFORM_MAX_12BIT_VALUE;
		}

		sineTable[i] = (uint32_t) (sample + 0.5f);
	}
}

/**
 * @brief Generates a 12-bit triangle lookup table.
 *
 * The waveform ramps from 0 to 4095, then back from 4095 to 0.
 */
static void DAC_Waveform_GenerateTriangleTable(void) {
	uint32_t halfSampleCount = DAC_WAVEFORM_SAMPLE_COUNT / 2U;

	for (uint32_t i = 0; i < DAC_WAVEFORM_SAMPLE_COUNT; i++) {
		if (i < halfSampleCount) {
			triangleTable[i] = (DAC_WAVEFORM_MAX_12BIT_VALUE * i)
					/ (halfSampleCount - 1U);
		} else {
			triangleTable[i] = (DAC_WAVEFORM_MAX_12BIT_VALUE
					* (DAC_WAVEFORM_SAMPLE_COUNT - 1U - i))
					/ (DAC_WAVEFORM_SAMPLE_COUNT - halfSampleCount - 1U);
		}
	}
}

/**
 * @brief Generates all waveform tables.
 */
static void DAC_Waveform_GenerateTables(void) {
	DAC_Waveform_GenerateSineTable();
	DAC_Waveform_GenerateTriangleTable();

	tablesGenerated = 1;
}

/**
 * @brief Configures TIM2 update frequency for the requested waveform frequency.
 *
 * waveform_frequency = timer_update_frequency / sample_count
 *
 * Therefore:
 *
 * timer_update_frequency = waveform_frequency * sample_count
 *
 * @param frequencyHz Desired waveform frequency in Hz.
 * @return DAC_WAVEFORM_OK if successful.
 */
static DAC_Waveform_Status_t DAC_Waveform_ConfigTimer(uint32_t frequencyHz) {
	uint32_t timerClockHz;
	uint32_t updateFrequencyHz;
	uint32_t autoreloadValue;

	if ((frequencyHz < DAC_WAVEFORM_MIN_FREQUENCY_HZ)
			|| (frequencyHz > DAC_WAVEFORM_MAX_FREQUENCY_HZ)) {
		return DAC_WAVEFORM_INVALID_PARAM;
	}

	updateFrequencyHz = frequencyHz * DAC_WAVEFORM_SAMPLE_COUNT;
	timerClockHz = DAC_Waveform_GetTIM2ClockHz();

	if (updateFrequencyHz == 0U) {
		return DAC_WAVEFORM_INVALID_PARAM;
	}

	autoreloadValue = timerClockHz / updateFrequencyHz;

	if (autoreloadValue == 0U) {
		return DAC_WAVEFORM_INVALID_PARAM;
	}

	autoreloadValue -= 1U;

	HAL_TIM_Base_Stop(&htim2);

	__HAL_TIM_SET_PRESCALER(&htim2, 0U);
	__HAL_TIM_SET_AUTORELOAD(&htim2, autoreloadValue);
	__HAL_TIM_SET_COUNTER(&htim2, 0U);

	HAL_TIM_GenerateEvent(&htim2, TIM_EVENTSOURCE_UPDATE);

	return DAC_WAVEFORM_OK;
}

/**
 * @brief Starts DAC waveform generation with the given lookup table.
 *
 * @param waveformTable Pointer to the waveform table.
 * @param frequencyHz Desired waveform frequency in Hz.
 * @return DAC_WAVEFORM_OK if waveform generation starts successfully.
 */
static DAC_Waveform_Status_t DAC_Waveform_StartTable(uint32_t *waveformTable,
		uint32_t frequencyHz) {
	DAC_Waveform_Status_t status;

	if (waveformTable == NULL) {
		return DAC_WAVEFORM_INVALID_PARAM;
	}

	if (!tablesGenerated) {
		DAC_Waveform_GenerateTables();
	}

	status = DAC_Waveform_ConfigTimer(frequencyHz);

	if (status != DAC_WAVEFORM_OK) {
		return status;
	}

	HAL_TIM_Base_Stop(&htim2);
	HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);

	if (HAL_DAC_Start_DMA(&hdac,
	DAC_CHANNEL_1, waveformTable,
	DAC_WAVEFORM_SAMPLE_COUNT,
	DAC_ALIGN_12B_R) != HAL_OK) {
		return DAC_WAVEFORM_ERROR;
	}

	if (HAL_TIM_Base_Start(&htim2) != HAL_OK) {
		HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);
		return DAC_WAVEFORM_ERROR;
	}

	return DAC_WAVEFORM_OK;
}

/**
 * @brief Initializes the DAC waveform module.
 *
 * @return DAC_WAVEFORM_OK if initialization is successful.
 */
DAC_Waveform_Status_t DAC_Waveform_Init(void) {
	DAC_Waveform_GenerateTables();
	DAC_Waveform_Stop();

	return DAC_WAVEFORM_OK;
}

/**
 * @brief Starts sine wave generation using DAC + DMA + TIM2 trigger.
 *
 * @param frequencyHz Desired sine wave frequency in Hz.
 * @return DAC_WAVEFORM_OK if sine generation starts successfully.
 */
DAC_Waveform_Status_t DAC_Waveform_StartSine(uint32_t frequencyHz) {
	return DAC_Waveform_StartTable(sineTable, frequencyHz);
}

/**
 * @brief Starts triangle wave generation using DAC + DMA + TIM2 trigger.
 *
 * @param frequencyHz Desired triangle wave frequency in Hz.
 * @return DAC_WAVEFORM_OK if triangle generation starts successfully.
 */
DAC_Waveform_Status_t DAC_Waveform_StartTriangle(uint32_t frequencyHz) {
	return DAC_Waveform_StartTable(triangleTable, frequencyHz);
}

/**
 * @brief Stops DAC waveform generation.
 *
 * @return DAC_WAVEFORM_OK if stopped successfully.
 */
DAC_Waveform_Status_t DAC_Waveform_Stop(void) {
	HAL_TIM_Base_Stop(&htim2);
	HAL_DAC_Stop_DMA(&hdac, DAC_CHANNEL_1);

	return DAC_WAVEFORM_OK;
}

/**
 * @brief Changes waveform frequency while DAC output is running.
 *
 * @param frequencyHz Desired waveform frequency in Hz.
 * @return DAC_WAVEFORM_OK if frequency is updated successfully.
 */
DAC_Waveform_Status_t DAC_Waveform_SetFrequency(uint32_t frequencyHz) {
	return DAC_Waveform_ConfigTimer(frequencyHz);
}
