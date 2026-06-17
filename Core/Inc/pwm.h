/*
 * pwm.h
 *
 *  Created on: Jun 13, 2026
 *      Author: Eylül Öztek
 */

#ifndef INC_PWM_H_
#define INC_PWM_H_

#include "main.h"

/**
 * @file pwm.h
 * @brief PWM control function declarations for STM32 HAL based projects.
 *
 * This file provides basic function prototypes for starting, stopping,
 * and configuring PWM output. The current implementation is designed
 * around TIM4 Channel 1 for frequency and duty cycle configuration.
 */

/**
 * @brief Starts PWM signal generation on the selected timer channel.
 *
 * This function is a wrapper around HAL_TIM_PWM_Start().
 *
 * @param htim Pointer to the TIM handle used for PWM generation.
 * @param channel Timer channel to start. Example: TIM_CHANNEL_1.
 */
void pwmChannelStart(TIM_HandleTypeDef *htim, uint32_t channel);

/**
 * @brief Stops PWM signal generation on the selected timer channel.
 *
 * This function is a wrapper around HAL_TIM_PWM_Stop().
 *
 * @param htim Pointer to the TIM handle used for PWM generation.
 * @param channel Timer channel to stop. Example: TIM_CHANNEL_1.
 */
void pwmChannelStop(TIM_HandleTypeDef *htim, uint32_t channel);

/**
 * @brief Sets the PWM frequency and duty cycle.
 *
 * This function updates TIM4 ARR and CCR1 registers directly.
 * It assumes that TIM4 is configured with a timer counter clock of 1 MHz.
 *
 * Frequency calculation:
 *
 * ARR = 1000000 / frequency
 *
 * Duty cycle calculation:
 *
 * CCR1 = ARR * dutyCycle / 100
 *
 * @param frequency Desired PWM frequency in Hz.
 * @param dutyCycle Desired duty cycle in percent. Valid range should be 0.0 to 100.0.
 *
 * @note This function is currently specific to TIM4 Channel 1.
 * @note The function does not check for zero frequency. Passing 0 causes division by zero.
 */
void setPWMFreqDuty(float frequency, float dutyCycle);

#endif /* INC_PWM_H_ */
