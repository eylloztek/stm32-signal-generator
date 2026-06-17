# STM32 Signal Generator

A simple STM32-based signal generator project using PWM, DAC, DMA, timer triggering, push buttons, and an SSD1306 OLED display.

This project can generate three basic waveform types:

* Square wave using TIM4 PWM
* Sine wave using DAC OUT1, DMA, and TIM2 trigger
* Triangle wave using DAC OUT1, DMA, and TIM2 trigger

The user interface is displayed on an SSD1306 OLED screen. The current version uses three push buttons for menu navigation and parameter adjustment.

> Note: This project uses push buttons for simplicity. However, for a more practical and user-friendly signal generator, a rotary encoder would be a better input method. A rotary encoder allows faster frequency and duty cycle adjustment, especially when changing values over a wide range.

---

## Features

* STM32 HAL-based implementation
* OLED menu interface
* Button-controlled menu navigation
* PWM square wave generation
* DAC sine wave generation
* DAC triangle wave generation
* Frequency adjustment
* Duty cycle adjustment for PWM mode
* Output ON/OFF control
* Software button debounce
* DAC waveform lookup tables
* DAC + DMA + TIM2 trigger-based analog waveform generation
* Independent Watchdog enabled in the main project

---

## Supported Waveforms

| Waveform     | Generation Method             | Output Type | Duty Cycle |
| ------------ | ----------------------------- | ----------- | ---------- |
| Square PWM   | TIM4 PWM Channel 1            | Digital PWM | Supported  |
| Sine DAC     | DAC OUT1 + DMA + TIM2 trigger | Analog DAC  | Not used   |
| Triangle DAC | DAC OUT1 + DMA + TIM2 trigger | Analog DAC  | Not used   |

---

## Hardware Used

* STM32 Nucleo-F446RE
* SSD1306 OLED display, I2C interface
* Push buttons
* Optional external measurement device or oscilloscope
* Jumper wires
* Breadboard

---

## Project Structure

```text
Core/
├── Inc/
│   ├── pwm.h
│   ├── menu.h
│   ├── dac_waveform.h
│   └── ...
│
├── Src/
│   ├── pwm.c
│   ├── menu.c
│   ├── dac_waveform.c
│   ├── main.c
│   └── ...
```

---

## File Descriptions

### `pwm.h` / `pwm.c`

These files contain helper functions for PWM output control.

Main functions:

```c
void pwmChannelStart(TIM_HandleTypeDef *htim, uint32_t channel);
void pwmChannelStop(TIM_HandleTypeDef *htim, uint32_t channel);
void setPWMFreqDuty(float frequency, float dutyCycle);
```

The current PWM implementation directly updates TIM4 registers:

```c
TIM4->ARR = (int)(1000000.0 / frequency);
TIM4->CCR1 = (int)((TIM4->ARR) * (dutyCycle / 100.0));
```

This assumes that TIM4 has a 1 MHz timer counter clock.

---

### `dac_waveform.h` / `dac_waveform.c`

These files generate sine and triangle waveforms using DAC, DMA, and TIM2.

Main functions:

```c
DAC_Waveform_Status_t DAC_Waveform_Init(void);
DAC_Waveform_Status_t DAC_Waveform_StartSine(uint32_t frequencyHz);
DAC_Waveform_Status_t DAC_Waveform_StartTriangle(uint32_t frequencyHz);
DAC_Waveform_Status_t DAC_Waveform_Stop(void);
DAC_Waveform_Status_t DAC_Waveform_SetFrequency(uint32_t frequencyHz);
```

The DAC waveform module uses lookup tables:

```c
static uint32_t sineTable[DAC_WAVEFORM_SAMPLE_COUNT];
static uint32_t triangleTable[DAC_WAVEFORM_SAMPLE_COUNT];
```

The waveform sample count is:

```c
#define DAC_WAVEFORM_SAMPLE_COUNT 100U
```

---

### `menu.h` / `menu.c`

These files manage the OLED menu interface and button-based user interaction.

Main menu items:

```text
Set Waveform
Set Frequency
Set DutyCycle
About
```

Supported waveform selection:

```text
Square PWM
Sine DAC
Triangle DAC
```

Button behavior:

| Button | Main Menu              | Setting Screens     |
| ------ | ---------------------- | ------------------- |
| MENU   | Move to next menu item | Increase value      |
| ON/OFF | Toggle output          | Decrease value      |
| SELECT | Enter submenu          | Return to main menu |

---

### `main.c`

The main file initializes the STM32 peripherals and runs the main program loop.

Main loop:

```c
while (1) {
    handleMenuNavigation();
    HAL_IWDG_Refresh(&hiwdg);
}
```

The output is not started automatically at startup. It is controlled from the menu using the ON/OFF button.

---

## STM32 Peripheral Configuration

### TIM4 PWM

TIM4 is used for square wave PWM generation.

In the current project:

```c
htim4.Init.Prescaler = 80 - 1;
htim4.Init.Period = 1000 - 1;
```

TIM4 Channel 1 is configured in PWM mode.

---

### DAC

DAC OUT1 is used for analog sine and triangle waveform output.

DAC configuration:

```c
sConfig.DAC_Trigger = DAC_TRIGGER_T2_TRGO;
sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
HAL_DAC_ConfigChannel(&hdac, &sConfig, DAC_CHANNEL_1);
```

The DAC output pin for DAC OUT1 on STM32F446RE is:

```text
PA4 / DAC_OUT1
```

Both sine and triangle waves are generated on the same DAC output channel.

---

### TIM2

TIM2 is used as the DAC trigger timer.

TIM2 master output trigger:

```c
sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
```

Each TIM2 update event triggers the DAC to output the next sample from the waveform table.

---

### DMA

DMA is used to transfer waveform samples from memory to the DAC data register.

For continuous waveform generation, the DAC DMA mode should be configured as circular mode in STM32CubeMX.

---

### Independent Watchdog Usage

This project uses the STM32 Independent Watchdog, also known as IWDG, to improve runtime reliability.

The watchdog is a hardware timer that resets the microcontroller if the software stops refreshing it within a configured time window. This is useful in embedded systems because it can recover the system from unexpected software lockups, infinite loops, peripheral blocking states, or other runtime faults.

In this project, the watchdog is initialized during startup:

```c
MX_IWDG_Init();
```

The watchdog is refreshed inside the main loop:

```c
while (1) {
    handleMenuNavigation();
    HAL_IWDG_Refresh(&hiwdg);
}
```

This means the program must keep running normally through the main loop. If the firmware gets stuck and cannot reach HAL_IWDG_Refresh(&hiwdg), the watchdog timer expires and the STM32 automatically resets.

#### Watchdog Configuration

The current IWDG configuration is:

```text
hiwdg.Instance = IWDG;
hiwdg.Init.Prescaler = IWDG_PRESCALER_256;
hiwdg.Init.Reload = 1249;
```

The Independent Watchdog is clocked from the LSI oscillator. The approximate timeout can be calculated with:

```text
IWDG timeout = (Reload + 1) × Prescaler / LSI frequency
```

Assuming an approximate LSI frequency of 32 kHz:

```text
IWDG timeout = (1249 + 1) × 256 / 32000
IWDG timeout ≈ 10 seconds
```

So, if the firmware does not refresh the watchdog for about 10 seconds, the MCU will reset.

#### Why Watchdog Is Useful in This Project

The project uses multiple peripherals:

* OLED display over I2C
* Timer-based PWM output
* DAC output
* DMA waveform transfer
* Button-based menu control
* TIM2-triggered DAC waveform generation

In embedded projects with several peripherals, a blocking peripheral call or unexpected software state may cause the program to freeze. The watchdog provides a basic recovery mechanism by resetting the system if the main loop stops running correctly.

#### Important Note About Long Delays

The splash screen currently uses a delay at startup. If the delay duration becomes longer than the watchdog timeout, the watchdog may reset the MCU before the main loop starts.

For long startup delays, either keep the delay shorter than the watchdog timeout or refresh the watchdog during the delay.

Example:

```c
for (uint8_t i = 0; i < 5; i++) {
    HAL_IWDG_Refresh(&hiwdg);
    HAL_Delay(1000);
}
```

This keeps the watchdog alive during a 5-second splash screen.

#### Watchdog Design Recommendation

The watchdog should not be refreshed inside every low-level driver function. It is usually better to refresh it from the main control loop or from a reliable scheduler task. This makes the watchdog more meaningful, because it confirms that the main application flow is still running.

In this project, refreshing the watchdog in the main loop is acceptable because the program structure is simple and menu-driven.

---

### OLED Display

The OLED display is driven over I2C using an SSD1306 driver.

It is used for:

* Splash screen
* Main menu
* Waveform selection
* Frequency setting
* Duty cycle setting
* About page
* Output warning messages

---

## Pin Configuration

Adjust the exact pins according to your STM32CubeMX configuration and `main.h` labels.

| Function      | STM32 Function / Label | Description                     |
| ------------- | ---------------------- | ------------------------------- |
| DAC Output    | PA4 / DAC_OUT1         | Analog sine and triangle output |
| PWM Output    | TIM4_CH1               | Square wave PWM output          |
| OLED SCL      | I2C1_SCL               | OLED clock line                 |
| OLED SDA      | I2C1_SDA               | OLED data line                  |
| MENU Button   | `MENU_BUTTON_Pin`      | Menu navigation / increment     |
| SELECT Button | `select_Pin`           | Enter / back                    |
| ON/OFF Button | `ONOFF_BUTTON_Pin`     | Output toggle / decrement       |
| GND           | GND                    | Common ground                   |

Button wiring in this project uses pull-up logic:

```text
Pressed  = GPIO_PIN_RESET
Released = GPIO_PIN_SET
```

This means each button should be connected between the GPIO pin and GND when internal pull-up is enabled.

---

## Menu Flow

```text
Main Menu
│
├── Set Waveform
│   ├── Square PWM
│   ├── Sine DAC
│   └── Triangle DAC
│
├── Set Frequency
│   ├── PWM frequency range
│   └── DAC waveform frequency range
│
├── Set DutyCycle
│   └── Only available in PWM mode
│
└── About
    ├── Current status
    ├── Output type information
    └── Button controls
```

The waveform, frequency, and duty cycle cannot be changed while the output is active. The output must be stopped first.

---

## Frequency Ranges

### PWM Mode

```c
#define PWM_FREQUENCY_MAX_HZ   100000U
#define PWM_FREQUENCY_STEP_HZ  100U
```

PWM frequency range:

```text
1 Hz to 100 kHz
```

### DAC Modes

```c
#define DAC_WAVEFORM_MAX_FREQUENCY_HZ 10000U
#define DAC_FREQUENCY_STEP_HZ         10U
```

DAC waveform frequency range:

```text
1 Hz to 10 kHz
```

The DAC frequency range is intentionally lower because DAC waveform generation depends on sample count and timer update rate.

---

## Important Formulas

### General STM32 Timer PWM Frequency Formula

For a timer configured in up-counting mode:

```text
PWM frequency = Timer clock / ((PSC + 1) × (ARR + 1))
```

Where:

* `PSC` is the timer prescaler
* `ARR` is the auto-reload register
* `Timer clock` is the input clock of the timer

In this project, TIM4 is configured so that the timer counter clock is approximately 1 MHz.

Therefore, the PWM period is controlled mainly by ARR.

The current project code uses:

```text
ARR = 1000000 / frequency
```

For a more exact STM32 timer calculation, the formula can be written as:

```text
ARR = (Timer counter clock / desired frequency) - 1
```

---

### PWM Duty Cycle Formula

The duty cycle is controlled by the capture/compare register.

General formula:

```text
Duty cycle (%) = (CCR / ARR) × 100
```

In the current project code:

```text
CCR1 = ARR × dutyCycle / 100
```

For example:

```text
frequency = 1000 Hz
dutyCycle = 50%
ARR = 1000000 / 1000 = 1000
CCR1 = 1000 × 50 / 100 = 500
```

This produces approximately a 1 kHz PWM signal with 50% duty cycle.

---

### DAC Resolution Formula

The STM32 DAC is used in 12-bit mode.

The DAC digital range is:

```text
0 to 4095
```

The analog output voltage is approximately:

```text
Vout = (DAC_Value / 4095) × Vref
```

If `Vref = 3.3V`:

```text
DAC_Value = 0     -> Vout ≈ 0V
DAC_Value = 2048  -> Vout ≈ 1.65V
DAC_Value = 4095  -> Vout ≈ 3.3V
```

The DAC cannot generate negative voltage directly. Therefore, sine and triangle waveforms are generated as offset waveforms between 0V and 3.3V.

---

### Sine Wave Lookup Table Formula

The sine wave table is generated with:

```text
sample[i] = 2048 + 2047 × sin(2πi / N)
```

Where:

* `i` is the sample index
* `N` is the number of samples
* `N = 100` in this project
* `2048` is the mid-scale offset
* `2047` is the amplitude

This produces a 12-bit sine wave table between approximately 0 and 4095.

---

### Triangle Wave Lookup Table Formula

The triangle wave ramps from 0 to 4095, then back from 4095 to 0.

For the rising half:

```text
sample[i] = 4095 × i / (N/2 - 1)
```

For the falling half:

```text
sample[i] = 4095 × (N - 1 - i) / (N - N/2 - 1)
```

Where:

* `i` is the sample index
* `N` is the number of samples
* `N = 100` in this project

---

### DAC Waveform Frequency Formula

The DAC outputs one sample at each TIM2 update event.

Therefore:

```text
Waveform frequency = TIM2 update frequency / sample count
```

Or:

```text
TIM2 update frequency = Waveform frequency × sample count
```

Since this project uses 100 samples:

```text
TIM2 update frequency = Waveform frequency × 100
```

Examples:

```text
1 kHz sine wave      -> TIM2 update frequency = 100 kHz
5 kHz triangle wave  -> TIM2 update frequency = 500 kHz
10 kHz DAC waveform  -> TIM2 update frequency = 1 MHz
```

This is why DAC waveform frequency is limited compared to PWM output frequency.

---

## How to Use

1. Power the STM32 board.
2. The splash screen appears on the OLED.
3. The main menu is displayed.
4. Use the MENU button to move between menu items.
5. Press SELECT to enter a menu.
6. Select a waveform from `Set Waveform`.
7. Set the desired frequency from `Set Frequency`.
8. If using PWM square wave, set duty cycle from `Set DutyCycle`.
9. Return to the main menu.
10. Press ON/OFF to start the output.
11. Press ON/OFF again to stop the output.

---

## Output Behavior

### Square PWM Mode

* Output is generated using TIM4 Channel 1.
* Frequency and duty cycle are both used.
* Output is digital PWM.

### Sine DAC Mode

* Output is generated using DAC OUT1.
* DMA continuously transfers sine table samples to the DAC.
* TIM2 controls the sample update rate.
* Duty cycle is not used.

### Triangle DAC Mode

* Output is generated using DAC OUT1.
* DMA continuously transfers triangle table samples to the DAC.
* TIM2 controls the sample update rate.
* Duty cycle is not used.

---

## Button-Based Control

This version uses three push buttons:

```text
MENU
SELECT
ON/OFF
```

This keeps the hardware simple and easy to test.

However, for a real signal generator, push buttons are not the most convenient input method. Frequency adjustment can become slow because the frequency range is wide.

For example:

```text
PWM frequency range: 1 Hz to 100000 Hz
PWM step size: 100 Hz
```

Changing values over a large range using only buttons requires many button presses.

---

## Recommended Improvement: Rotary Encoder

A rotary encoder is strongly recommended for a more practical version of this project.

A rotary encoder would improve the project because:

* Frequency adjustment would be faster.
* Duty cycle adjustment would feel more natural.
* A single encoder with push-button could replace multiple buttons.
* Encoder acceleration could be added for large frequency changes.
* The interface would be closer to a real bench signal generator.

Suggested encoder behavior:

| Encoder Action           | Function                |
| ------------------------ | ----------------------- |
| Rotate clockwise         | Increase selected value |
| Rotate counter-clockwise | Decrease selected value |
| Press encoder button     | Enter / confirm         |
| Long press               | Back / output toggle    |

A better user interface design would be:

```text
Rotary encoder rotation -> Change selected value
Encoder button press    -> Enter / confirm
Separate ON/OFF button  -> Start / stop output
```

This would make the project much more usable than the current button-only version.

---

## Testing Without an Oscilloscope

An oscilloscope is the best tool for observing the generated waveforms.

If an oscilloscope is not available, possible alternatives are:

### 1. LED Test

A low-frequency waveform can be roughly observed with an external LED and resistor.

Example:

```text
DAC_OUT1 / PA4 -> resistor -> LED -> GND
```

Use very low frequencies, such as:

```text
1 Hz to 2 Hz
```

This only shows brightness changes. It does not show the real waveform shape.

---

### 2. DAC-to-ADC Feedback Test

A better method is to connect DAC OUT1 to an ADC input:

```text
PA4 / DAC_OUT1 -> ADC input pin
```

Then the STM32 can read back the generated analog signal and send the sampled values to a PC over UART.

The values can be plotted using a serial plotter.

This is useful for checking the rough shape of sine and triangle waves without an oscilloscope.

---

## Limitations

* DAC output is limited to 0V–3.3V.
* The DAC cannot generate negative voltage directly.
* DAC sine and triangle waves are offset waveforms, not true bipolar AC signals.
* Button-based control is usable but not ideal for wide-range adjustment.
* PWM function is currently specific to TIM4 Channel 1.
* `setPWMFreqDuty()` does not check for zero frequency.
* DAC waveform quality depends on sample count and timer update frequency.
* At high DAC waveform frequencies, waveform smoothness decreases.
* The project does not currently include amplitude control or offset control.
* The project does not include a true analog output buffer stage.

---

## Possible Improvements

* Add rotary encoder support.
* Add encoder acceleration.
* Add amplitude control.
* Add DC offset control.
* Add serial plotter debug mode.
* Add ADC feedback waveform viewer.
* Add UART command interface.
* Add OLED mini waveform preview.
* Add EEPROM setting storage.
* Add more waveform types, such as sawtooth wave.
* Add safer parameter validation for PWM frequency and duty cycle.
* Make the PWM driver more generic by passing timer handle, channel, and timer clock as parameters.
