/*
Author: Qasim Shahid
This file implements all the functions defined in bbbio.h. It uses a filesystem implementation to interface with the Beaglebone Black's various pins. 
Current functionality supports: GPIO, PWM

ALL COMMENTS FOR THE FUNCTIONS ARE IN BBBIO.H AND WILL NOT BE REPEATED HERE.
PLEASE CHECK BBBBIO.H TO SEE HOW TO USE THESE FUNCTIONS.
*/


#include "bbbio.h" 


static int32_t file_exists(Buffer file_path) {
    int32_t result  = 0;

    // Check the file exists
    if (access((char *) file_path, F_OK) != -1) {
        result = 1;
    }
    
    return result;
}


static int32_t write_to_file(Buffer file_path, Buffer value) {
    int32_t result = 0;

    // Ensure file path and value are valid (not empty)
    if (file_path != NULL && value != NULL && file_path[0] != '\0' && value[0] != '\0') {

        FILE *file = fopen((char *) file_path, "w");
        if (file != NULL) {
            if (fprintf(file, "%s", value) > 0) {
                result = 1;  // Successfully wrote to the file
            }
    
            int32_t u = fclose(file);
        }
    }

    return result;
}


static int32_t write_to_file_int(Buffer file_path, int32_t value) {
    int32_t result = 0;
    Buffer value_str;

    // Convert the integer value to a string
    if (snprintf((char *) value_str, sizeof(value_str), "%d", value) > 0) {
        // Call the other function to write to the file
        result = write_to_file(file_path, value_str);
    }

    return result;
}


static int32_t read_from_file(Buffer file_path, Buffer buff) {
    int32_t result = 0;

    // Ensure file path and buffer are valid (not empty)
    if (file_path != NULL && buff != NULL && file_path[0] != '\0') {

        FILE *file = fopen((char *) file_path, "r");
        if (file != NULL) {
            if (fgets((char *) buff, MAX_WRITE_LENGTH, file) != NULL) {
                result = 1;  // Successfully read from the file
            }
    
            int32_t u = fclose(file);
        }
    }

    return result;
}


int32_t write_gpio_value(int32_t pin, int32_t value) {
    int32_t result = 0;
    Buffer value_file_path; 

    // If we were able to successfully create the file path, try to write to it. 
    if (snprintf((char *) value_file_path, sizeof(value_file_path), GPIO_VALUE_PATH, pin) > 0) {

        result = write_to_file_int(value_file_path, value);
    }

    return result;
}


int32_t setup_gpio_pin(int32_t pin, Buffer direction) {
    int32_t result = 0;
    Buffer value_file_path;

    if (snprintf((char *) value_file_path, sizeof(value_file_path), GPIO_VALUE_PATH, pin) > 0) {

        if (file_exists(value_file_path) == 1) {
            result = 1;  // File already exists, pin already exported
        }
        else {
            // Value file doesn't exist, so we need to export the pin
            result = write_to_file_int((BufferPointer) GPIO_EXPORT_PATH, pin);

            if (result == 1) {
                int32_t u = usleep(500000);
            }
        }
    }

    // Set the direction
    if (result == 1) {
        Buffer direction_file_path;
        if (snprintf((char *) direction_file_path, sizeof(direction_file_path), GPIO_DIRECTION_PATH, pin) > 0) {
            result = write_to_file(direction_file_path, direction);
        }
    }

    return result;
}


void set_gpio_on(int32_t pin) {
    int32_t result = write_gpio_value(pin, GPIO_ON);
}


void set_gpio_off(int32_t pin) {
    int32_t result = write_gpio_value(pin, GPIO_OFF);
}


int32_t read_gpio_value(int32_t pin) {
    int32_t result = -1;
    Buffer value_file_path;
    Buffer buff;

    // Create the file path for the GPIO value
    if (snprintf((char *)value_file_path, sizeof(value_file_path), GPIO_VALUE_PATH, pin) > 0) {
        if (read_from_file(value_file_path, buff) == 1) {

            // Check the value read from the file
            if (buff[0] == '1') {
                result = 1;  // GPIO value is 1
            } 
            else if (buff[0] == '0') {
                result = 0;  // GPIO value is 0
            }
            else {
                result = -1;
            }
        }
    }

    return result;
}


void set_pwm_enable(Buffer pin_identifier, int32_t value) {
    int32_t result = 0;
    BufferPointer channel_path = (BufferPointer) NULL_STR;

    if (pin_identifier[0] == '1' && pin_identifier[1] == 'A') {
        channel_path = (BufferPointer) PWM1PINA_PATH;
    } 
    else if (pin_identifier[0] == '1' && pin_identifier[1] == 'B') {
        channel_path = (BufferPointer) PWM1PINB_PATH;
    } 
    else if (pin_identifier[0] == '2' && pin_identifier[1] == 'A') {
        channel_path = (BufferPointer) PWM2PINA_PATH;
    } 
    else if (pin_identifier[0] == '2' && pin_identifier[1] == 'B') {
        channel_path = (BufferPointer) PWM2PINB_PATH;
    }
    else {
        channel_path = (BufferPointer) NULL_STR;
    }

    if (strncmp((char *) channel_path, (char *) NULL_STR, sizeof(NULL_STR)) == 0) {
        result = 0;
    }
    else {
        Buffer enable_path;
        if (snprintf((char *)enable_path, sizeof(enable_path), "%s%s", (char *)channel_path, PWM_ENABLE_PATH) > 0) {
            result = write_to_file_int(enable_path, value);
        }
    }
}


void set_pwm_duty_cycle(Buffer pin_identifier, int32_t frequency, float32_t duty_percent) {
    int32_t result = 0;
    BufferPointer channel_path = (BufferPointer) NULL_STR;

    if (pin_identifier[0] == '1' && pin_identifier[1] == 'A') {
        channel_path = (BufferPointer) PWM1PINA_PATH;
    } 
    else if (pin_identifier[0] == '1' && pin_identifier[1] == 'B') {
        channel_path = (BufferPointer) PWM1PINB_PATH;
    } 
    else if (pin_identifier[0] == '2' && pin_identifier[1] == 'A') {
        channel_path = (BufferPointer) PWM2PINA_PATH;
    } 
    else if (pin_identifier[0] == '2' && pin_identifier[1] == 'B') {
        channel_path = (BufferPointer) PWM2PINB_PATH;
    }
    else {
        channel_path = (BufferPointer) NULL_STR;
    }

    if (strncmp((char *) channel_path, (char*) NULL_STR, sizeof(NULL_STR)) == 0) {
        result = 0;
    }
    else {
        int32_t  period_ns = -1;
        int32_t duty_ns = -1;

        if (frequency <= 0 || (int) (duty_percent <= 0.0f) || (int) (duty_percent > 100.0f)) {
            result = 0;
        }
        else {
            period_ns = (int32_t)(1000000000.0f / frequency);
            duty_ns = (period_ns * (duty_percent / 100.0f));
        }

        // Write the duty cycle to the file
        if (duty_ns >= 0 && (int) (period_ns >= 0)) {
            Buffer duty_cycle_path;
            if (snprintf((char *)duty_cycle_path, sizeof(duty_cycle_path), "%s%s", (char *)channel_path, PWM_DUTY_CYCLE_PATH) > 0) {
                result = write_to_file_int(duty_cycle_path, duty_ns);
            }
        }
    }
}


void set_pwm_frequency(Buffer pin_identifier, int32_t frequency) {
    int32_t result = 0;
    BufferPointer channel_path = (BufferPointer) NULL_STR;

    if (pin_identifier[0] == '1' && pin_identifier[1] == 'A') {
        channel_path = (BufferPointer) PWM1PINA_PATH;
    } 
    else if (pin_identifier[0] == '1' && pin_identifier[1] == 'B') {
        channel_path = (BufferPointer) PWM1PINB_PATH;
    } 
    else if (pin_identifier[0] == '2' && pin_identifier[1] == 'A') {
        channel_path = (BufferPointer) PWM2PINA_PATH;
    } 
    else if (pin_identifier[0] == '2' && pin_identifier[1] == 'B') {
        channel_path = (BufferPointer) PWM2PINB_PATH;
    }
    else {
        channel_path = (BufferPointer) NULL_STR;
    }

    if (strncmp((char *) channel_path, (char *)NULL_STR, sizeof(NULL_STR)) == 0) {
        result = 0;
    }
    else {
        int32_t period_ns = -1;

        if (frequency <= 0) {
            result = 0;
        }
        else {
            period_ns = (int32_t)(1000000000.0f / frequency);
        }

        if (period_ns >= 0) {
            Buffer period_path;
            if (snprintf((char *)period_path, sizeof(period_path), "%s%s", (char *)channel_path, PWM_PERIOD_PATH) > 0) {
                result = write_to_file_int(period_path, period_ns);
            }
        }
    }
}


int32_t setup_pwm(Buffer pin_identifier, int32_t frequency, float32_t duty_percent) {

    int32_t result = 1;           // Default to success; will clear on error
    Buffer file_path;
    BufferPointer channel_path = (BufferPointer) NULL_STR;
    int32_t channel_number = -1;
    int32_t period_ns = 0;
    int32_t duty_ns = 0;
    
    // Validate duty_percent and frequency
    if ((int) (duty_percent <= 0.0f) || (int) (duty_percent > 100.0f) || frequency <= 0) {
        result = 0;
    }
    else {
        period_ns = (int32_t)(1000000000.0f / frequency);
        duty_ns = (period_ns * (duty_percent / 100.0f));
    }
    
    // Determine channel based on subsystem and pin identifier.
    if (result == 1) {
        if (pin_identifier[0] == '1' && pin_identifier[1] == 'A') {
            channel_path = (BufferPointer) PWM1PINA_PATH;
            channel_number = 0;

            // Configure pin to pwm mode - config-pin {PIN} pwm
            result = write_to_file((BufferPointer) PWM1PINA_STATE_PATH, (BufferPointer) PWM_STATE);
        } 
        else if (pin_identifier[0] == '1' && pin_identifier[1] == 'B') {
            channel_path = (BufferPointer) PWM1PINB_PATH;
            channel_number = 1;

            result = write_to_file((BufferPointer) PWM1PINB_STATE_PATH, (BufferPointer) PWM_STATE);

        } 
        else if (pin_identifier[0] == '2' && pin_identifier[1] == 'A') {
            channel_path = (BufferPointer) PWM2PINA_PATH;
            channel_number = 0;

            result = write_to_file((BufferPointer) PWM2PINA_STATE_PATH, (BufferPointer) PWM_STATE);
        } 
        else if (pin_identifier[0] == '2' && pin_identifier[1] == 'B') {
            channel_path = (BufferPointer) PWM2PINB_PATH;
            channel_number = 1;

            result = write_to_file((BufferPointer) PWM2PINB_STATE_PATH, (BufferPointer) PWM_STATE);
        }
        else {
            result = 0;
        }
    } 

    // Export the channel if it is not already exported
    if (result == 1 && (int) (channel_number < 0)) {
        result = 0;
    }
    else {
        // Check if the channel is already exported
        if (file_exists(channel_path) == 1) {
            result = 1;  // File already exists, channel already exported
        }
        else {
            if (pin_identifier[0] == '1') {
                if (write_to_file_int((BufferPointer) PWM1_EXPORT_PATH, channel_number) != 1) {
                    result = 0;
                }
                else {
                    int32_t u = usleep(500000);
                    result = 1;
                }
            } 
            else if (pin_identifier[0] == '2') {
                if (write_to_file_int((BufferPointer) PWM2_EXPORT_PATH, channel_number) != 1) {
                    result = 0;
                }
                else {
                    int32_t u = usleep(500000);
                    result = 1;
                }
            }
            else {
                result = 0;
            }
        }
    }    

    if (result == 1) {
        set_pwm_frequency(pin_identifier, frequency);
        set_pwm_duty_cycle(pin_identifier, frequency, duty_percent);
        set_pwm_enable(pin_identifier, PWM_ON);
        int32_t u = usleep(500000);
    }
    
    return result;
}

 