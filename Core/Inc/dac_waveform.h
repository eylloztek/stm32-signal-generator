/*
 * dac_waveform.h
 *
 *  Created on: Jun 17, 2026
 *      Author: Eylül Öztek
 */

#ifndef INC_DAC_WAVEFORM_H_
#define INC_DAC_WAVEFORM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

#define DAC_WAVEFORM_SAMPLE_COUNT        100U

#define DAC_WAVEFORM_MIN_FREQUENCY_HZ    1U
#define DAC_WAVEFORM_MAX_FREQUENCY_HZ    10000U

typedef enum {
	DAC_WAVEFORM_OK = 0, DAC_WAVEFORM_ERROR, DAC_WAVEFORM_INVALID_PARAM
} DAC_Waveform_Status_t;

DAC_Waveform_Status_t DAC_Waveform_Init(void);

DAC_Waveform_Status_t DAC_Waveform_StartSine(uint32_t frequencyHz);
DAC_Waveform_Status_t DAC_Waveform_StartTriangle(uint32_t frequencyHz);

DAC_Waveform_Status_t DAC_Waveform_Stop(void);
DAC_Waveform_Status_t DAC_Waveform_SetFrequency(uint32_t frequencyHz);

#ifdef __cplusplus
}
#endif

#endif /* INC_DAC_WAVEFORM_H_ */
