/*
Author: Qasim Shahid
This file is for defining constants and function prototypes related to IO (input output) operations with the BeagleBone Black./
Why? Best practices in C: Define headers with constants and function prototypes.
Create a separate gpio.c file for the actual implementation (separation of concerns, this is kind of like using an interface in Java/C#)
This file is for defining constants and function prototypes related to IO operations with the BeagleBone Black.

Sources: 
https://vadl.github.io/beagleboneblack/2016/07/29/setting-up-bbb-gpio
This source was used to guide the definition of this header file. It provides instructions on how to interface with the GPIO pins on the BBB through the filesystem.
*/

#ifndef BBBIO_H
#define BBBIO_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>

/* --------------------------------------------- CONSTANTS ---------------------------------------------*/




/// ----------- MISC CONSTANTS ----------- ///
#define FILE_PATH_LENGTH 200

typedef uint8_t* BufferPointer;

#define NULL_STR "NULL"

// Usually we would use char here, but MISRA prefers an unsigned integer... Should work the same way, as char and uint8_t are both 1 byte / 8 bits with possible values of 0-255.
typedef uint8_t Buffer[FILE_PATH_LENGTH];

// Maximum characters we can write to a file with this library.
#define MAX_WRITE_LENGTH ((int32_t) 100) 

typedef float float32_t;



/// ----------- GPIO CONSTANTS ----------- ///
// The GPIO path for the BBB.
#define GLOBAL_GPIO_PATH "/sys/class/gpio/" 

// The value for the off state of a gpio pin (low voltage). 
#define GPIO_OFF ((int32_t) 0) 

// The value for the on state of a gpio pin (high voltage).
#define GPIO_ON ((int32_t) 1) 

// Input mode for the GPIO pin.
#define GPIO_INPUT_MODE "in"

// Output mode for the GPIO pin.
#define GPIO_OUTPUT_MODE "out"

// Concatenate the global GPIO path and the direction path to create a modular path to represent the path for the direction file for all GPIO pins.
// This way, if the global path changes, we only have to change it in the GLOBAL_GPIO_PATH macro.
#define GPIO_DIRECTION_PATH GLOBAL_GPIO_PATH "gpio%d/direction"

// Same principle as above, but for the value file.
#define GPIO_VALUE_PATH GLOBAL_GPIO_PATH "gpio%d/value"

// The GPIO Export path for the BBB.
#define GPIO_EXPORT_PATH GLOBAL_GPIO_PATH "export"




/// ----------- PWM CONSTANTS ----------- ///
#define DEVICES_PATH "/sys/devices/platform/ocp/"

#define PWM_BASE_PATH_TEMPLATE(EPWMSS_ADDR, PWM_ADDR, CHIP) DEVICES_PATH EPWMSS_ADDR ".epwmss/" PWM_ADDR ".pwm/pwm/pwmchip" CHIP "/"

// IMPORTANT: This is the PWM chip for the PWM Subsystem 1. This might be different on other versions of the image for BBB so make sure you have the correct one. 
// See technical reference manual for the AM335x processor for more information.
// https://www.ti.com/lit/ug/spruh73q/spruh73q.pdf
#define PWM1_CHIP "4"

// IMPORTANT: This is the PWM chip for the PWM Subsystem 2. This might be different on other versions of the image for BBB so make sure you have the correct one. 
// See technical reference manual for the AM335x processor for more information.
// https://www.ti.com/lit/ug/spruh73q/spruh73q.pdf
#define PWM2_CHIP "7"

// Memory locations for the PWM subsystem 1 on the BBB.
// These were found in the technical reference manual for the AM335x processor.
// https://www.ti.com/lit/ug/spruh73q/spruh73q.pdf
#define PWM1_BASE_PATH PWM_BASE_PATH_TEMPLATE("48302000", "48302200", PWM1_CHIP)

// Memory locations for the PWM subsystem 2 on the BBB.
// These were found in the technical reference manual for the AM335x processor.
// https://www.ti.com/lit/ug/spruh73q/spruh73q.pdf
#define PWM2_BASE_PATH PWM_BASE_PATH_TEMPLATE("48304000", "48304200", PWM2_CHIP)

#define PWM1_EXPORT_PATH PWM1_BASE_PATH "export"

#define PWM2_EXPORT_PATH PWM2_BASE_PATH "export"

#define PWM1PINA_PATH PWM1_BASE_PATH "pwm-" PWM1_CHIP ":0/"

#define PWM1PINB_PATH PWM1_BASE_PATH "pwm-" PWM1_CHIP ":1/"

#define PWM2PINA_PATH PWM2_BASE_PATH "pwm-" PWM2_CHIP ":0/"

#define PWM2PINB_PATH PWM2_BASE_PATH "pwm-" PWM2_CHIP ":1/"

#define PWM_DUTY_CYCLE_PATH "duty_cycle"

#define PWM_PERIOD_PATH "period"

#define PWM_ENABLE_PATH "enable"

// The following are based on the BeagleBone Pinout Map. EHRPWM1 and EHRPWM2 are the PWM subsystems on the BBB.
#define PWM1PINA_PIN_NAME "P9_14"

#define PWM1PINB_PIN_NAME "P9_16"

#define PWM2PINA_PIN_NAME "P8_19"

#define PWM2PINB_PIN_NAME "P8_13"

#define PWM1PINA_STATE_PATH DEVICES_PATH "ocp:" PWM1PINA_PIN_NAME "_pinmux/state"

#define PWM1PINB_STATE_PATH DEVICES_PATH "ocp:" PWM1PINB_PIN_NAME "_pinmux/state"

#define PWM2PINA_STATE_PATH DEVICES_PATH "ocp:" PWM2PINA_PIN_NAME "_pinmux/state"

#define PWM2PINB_STATE_PATH DEVICES_PATH "ocp:" PWM2PINB_PIN_NAME "_pinmux/state"

#define PWM_STATE "pwm"

#define PWM_ON ((int32_t) 1)

#define PWM_OFF ((int32_t) 0)




/* --------------------------------------------- FUNCTIONS ---------------------------------------------*/


// Description: Writes a value (0 or 1) to the specified GPIO pin.
// Parameters: 
// pin - The GPIO pin number
// value - The value to write (usually 0 or 1)
int32_t write_gpio_value(int32_t pin, int32_t value);


// Description: Exports the specified GPIO pin and sets the direction.
// Parameters:
// pin - The GPIO pin number
// direction - The direction to set for the GPIO. Use GPIO_INPUT_MODE or GPIO_OUTPUT_MODE macros.
// Returns - Returns 1 on successful setup, 0 on failure.
int32_t setup_gpio_pin(int32_t pin, Buffer direction);


// Description: Set the GPIO pin to high voltage / on state.
// Parameters: pin - The GPIO pin number
void set_gpio_on(int32_t pin);


// Description: Set the GPIO pin to low voltage / off state.
// Parameters: pin - The GPIO pin number
void set_gpio_off(int32_t pin);


// Description: Reads the value of the specified GPIO pin.
// Parameters: 
// pin - The GPIO pin number
// Returns - Returns 1 if the value is 1, 0 if the value is 0, -1 on failure.
int32_t read_gpio_value(int32_t pin);


// Description: Sets the duty cycle of the specified PWM channel.
// Parameters:
// pin_identifier - The pin identifier for the PWM channel (e.g. "1A", "1B", "2A", "2B")
// frequency      - Frequency in Hz
// duty_percent   - Duty cycle percentage (must be > 0 and <= 100)
void set_pwm_duty_cycle(Buffer pin_identifier, int32_t frequency, float32_t duty_percent);


// Description: Sets the frequency of the specified PWM channel.
// Parameters:
// pin_identifier - The pin identifier for the PWM channel (e.g. "1A", "1B", "2A", "2B")
// frequency      - Frequency in Hz
void set_pwm_frequency(Buffer pin_identifier, int32_t frequency);


// Description: Enables or disables the specified PWM channel.
// Parameters:
// pin_identifier - The pin identifier for the PWM channel (e.g. "1A", "1B", "2A", "2B")
// value          - 1 to enable, 0 to disable
void set_pwm_enable(Buffer pin_identifier, int32_t value);


// Description: Sets up the specified PWM channel with the given frequency (in Hz) and duty cycle (as a percentage).
// Parameters:
// pin_identifier - The pin identifier for the PWM channel (e.g. "1A", "1B", "2A", "2B")
// frequency      - Frequency in Hz
// duty_percent   - Duty cycle percentage (must be > 0 and <= 100)
int32_t setup_pwm(Buffer pin_identifier, int32_t frequency, float32_t duty_percent);


#endif // End of include guard


