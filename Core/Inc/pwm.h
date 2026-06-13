/*
 * pwm.h
 *
 *  Created on: Jun 13, 2026
 *      Author: Eylül Öztek
 */

#ifndef INC_PWM_H_
#define INC_PWM_H_

#include "main.h"

void pwmChannelStart(TIM_HandleTypeDef *htim, uint32_t channel);
void pwmChannelStop(TIM_HandleTypeDef *htim, uint32_t channel);
void setPWMFreqDuty(float frequency, float dutyCycle);

#endif /* INC_PWM_H_ */
