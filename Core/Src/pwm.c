/*
 * pwm.c
 *
 *  Created on: Jun 13, 2026
 *      Author: Eylül Öztek
 */

#include "pwm.h"

/**
 * @file pwm.c
 * @brief PWM control function implementations for STM32 HAL based projects.
 *
 * This source file contains basic helper functions for starting,
 * stopping, and configuring PWM output.
 */

/**
 * @brief Starts PWM signal generation on the selected timer channel.
 *
 * This function calls HAL_TIM_PWM_Start() with the given timer handle
 * and channel parameter.
 *
 * @param htim Pointer to the TIM handle used for PWM generation.
 * @param channel Timer channel to start. Example: TIM_CHANNEL_1.
 */
void pwmChannelStart(TIM_HandleTypeDef *htim, uint32_t channel) {
	HAL_TIM_PWM_Start(htim, channel);
}

/**
 * @brief Stops PWM signal generation on the selected timer channel.
 *
 * This function calls HAL_TIM_PWM_Stop() with the given timer handle
 * and channel parameter.
 *
 * @param htim Pointer to the TIM handle used for PWM generation.
 * @param channel Timer channel to stop. Example: TIM_CHANNEL_1.
 */
void pwmChannelStop(TIM_HandleTypeDef *htim, uint32_t channel) {
	HAL_TIM_PWM_Stop(htim, channel);
}

/**
 * @brief Sets the PWM frequency and duty cycle for TIM4 Channel 1.
 *
 * This function directly updates TIM4 auto-reload register (ARR)
 * and capture/compare register 1 (CCR1).
 *
 * The calculation assumes that TIM4 has a 1 MHz timer counter clock.
 * With this assumption, the PWM period is calculated as:
 *
 * ARR = 1000000 / frequency
 *
 * The duty cycle is calculated as:
 *
 * CCR1 = ARR * dutyCycle / 100
 *
 * @param frequency Desired PWM frequency in Hz.
 * @param dutyCycle Desired duty cycle in percent. Valid range should be 0.0 to 100.0.
 *
 * @note This function is specific to TIM4 Channel 1.
 * @note Make sure that frequency is not 0 before calling this function.
 * @note Make sure that dutyCycle is within 0.0 to 100.0.
 */
void setPWMFreqDuty(float frequency, float dutyCycle) {
	TIM4->ARR = (int) (1000000.0 / frequency);
	TIM4->CCR1 = (int) ((TIM4->ARR) * (dutyCycle / 100.0));
}
