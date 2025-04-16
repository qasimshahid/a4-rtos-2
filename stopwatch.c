#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include <sched.h>
#include "bbbio.h"

// Thread priorities higher number = higher priority
#define BUTTON_THREAD_PRIORITY     90  
#define DISPLAY_THREAD_PRIORITY    80  
#define TIMER_THREAD_PRIORITY      70  

int32_t START_STOP_BUTTON_PIN = -1;
int32_t RESET_BUTTON_PIN = -1;
int32_t RED_LED_PIN = -1;
int32_t GREEN_LED_PIN = -1;

float32_t current_time = 0.0f;       
int32_t stopwatch_running = 0;  
int32_t reset_requested = 0;

pthread_mutex_t stopwatch_mutex;

/**
 * Button thread function
 * Reads button states every 10ms and updates stopwatch state accordingly
 */
void *button_thread_func(void *arg) {
    int start_stop_prev = 0;
    int reset_prev = 0;
    int start_stop_current, reset_current;
    
    while (1) {
        // Read current button states using bbbio library
        start_stop_current = read_gpio_value(START_STOP_BUTTON_PIN);
        reset_current = read_gpio_value(RESET_BUTTON_PIN);
        
        // Check for start/stop button press (detect rising edge)
        if (start_stop_current == 1 && start_stop_prev == 0) {
            pthread_mutex_lock(&stopwatch_mutex);
            
            // Toggle stopwatch state
            stopwatch_running = !stopwatch_running;
            
            // Update LEDs based on state using bbbio functions
            if (stopwatch_running) {
                set_gpio_off(RED_LED_PIN);
                set_gpio_on(GREEN_LED_PIN);
            } else {
                set_gpio_on(RED_LED_PIN);
                set_gpio_off(GREEN_LED_PIN);
            }
            
            pthread_mutex_unlock(&stopwatch_mutex);
        }
        
        // Check for reset button press (detect rising edge)
        if (reset_current == 1 && reset_prev == 0) {
            pthread_mutex_lock(&stopwatch_mutex);
            reset_requested = true;
            pthread_mutex_unlock(&stopwatch_mutex);
        }
        
        // Update previous button states
        start_stop_prev = start_stop_current;
        reset_prev = reset_current;
        
        // Sleep for 10ms (button reading period)
        usleep(10000);
    }
    
    return NULL;
}

/**
 * Display thread function
 * Updates the terminal display every 100ms
 */
void *display_thread_func(void *arg) {
    float time_to_display;
    bool is_running;
    
    while (1) {
        // Get current state under mutex protection
        pthread_mutex_lock(&stopwatch_mutex);
        time_to_display = current_time;
        is_running = stopwatch_running;
        pthread_mutex_unlock(&stopwatch_mutex);
        
        // Clear the current line
        printf("\r                                        \r");
        
        // Display with appropriate resolution based on state
        if (is_running) {
            // Display with 100ms resolution when running
            printf("Time: %.1f seconds", time_to_display);
        } else {
            // Display with 10ms resolution when stopped
            printf("Time: %.2f seconds", time_to_display);
        }
        
        // Ensure output is displayed immediately
        fflush(stdout);
        
        // Sleep for 100ms (display update period)
        usleep(100000);
    }
    
    return NULL;
}

/**
 * Timer thread function
 * Updates the stopwatch counter every 10ms when the stopwatch is running
 */
void *timer_thread_func(void *arg) {
    struct timeval last_time, current_time_val;
    float elapsed_time;
    
    // Get initial time
    gettimeofday(&last_time, NULL);
    
    while (1) {
        // Get current time
        gettimeofday(&current_time_val, NULL);
        
        // Calculate elapsed time in seconds
        elapsed_time = (current_time_val.tv_sec - last_time.tv_sec) + 
                       (current_time_val.tv_usec - last_time.tv_usec) / 1000000.0f;
        
        // Update last time
        last_time = current_time_val;
        
        pthread_mutex_lock(&stopwatch_mutex);
        
        // Handle reset request if present
        if (reset_requested) {
            current_time = 0.0f;
            reset_requested = false;
        } 
        // Update current time if stopwatch is running
        else if (stopwatch_running) {
            current_time += elapsed_time;
            
            // Check for maximum floating point value and roll over if needed
            if (current_time >= FLT_MAX) {
                current_time = 0.0f;
            }
        }
        
        pthread_mutex_unlock(&stopwatch_mutex);
        
        // Sleep for approximately 10ms
        usleep(10000);
    }
    
    return NULL;
}

/**
 * Set up GPIO pins using the bbbio library
 */
int32_t gpio_setup() {

    // Initialize as 0 for unsuccessful until we make sure we setup our GPIO pins properly.
    int32_t ret = 0;
    if 
    ( 
        setup_gpio_pin(START_STOP_BUTTON_PIN, GPIO_INPUT_MODE) && 
        setup_gpio_pin(RESET_BUTTON_PIN, GPIO_INPUT_MODE) && 
        setup_gpio_pin(RED_LED_PIN, GPIO_OUTPUT_MODE) && 
        setup_gpio_pin(GREEN_LED_PIN, GPIO_OUTPUT_MODE)
    ) 
    {
        set_gpio_on(RED_LED_PIN);
        set_gpio_off(GREEN_LED_PIN);
        ret = 1;
    }
    else {
        ret = 0;
    }

    return ret;
}

/**
 * Set the value of a GPIO pin using bbbio library
 */
void gpio_set_value(int pin, int value) {
    if (value == 0) {
        set_gpio_off(pin);
    } else {
        set_gpio_on(pin);
    }
}

/**
 * Get the value of a GPIO pin using bbbio library
 */
int gpio_get_value(int pin) {
    return read_gpio_value(pin);
}

/**
 * Clean up resources before exiting
 */
void cleanup() {
    // Set all LEDs to off state before exit
    set_gpio_off(RED_LED_PIN);
    set_gpio_off(GREEN_LED_PIN);
    
    // Destroy mutex
    pthread_mutex_destroy(&stopwatch_mutex);
    
    printf("\nStopwatch application terminated.\n");
    exit(0);
}

int main() {

    // Set up threads with real-time priority using FIFO.
    pthread_t button_thread, display_thread, timer_thread;
    pthread_attr_t button_attr, display_attr, timer_attr;
    struct sched_param button_param, display_param, timer_param;

    pthread_attr_init(&button_attr);
    pthread_attr_init(&display_attr);
    pthread_attr_init(&timer_attr);

    pthread_attr_setschedpolicy(&button_attr, SCHED_FIFO);
    pthread_attr_setschedpolicy(&display_attr, SCHED_FIFO);
    pthread_attr_setschedpolicy(&timer_attr, SCHED_FIFO);

    button_param.sched_priority = BUTTON_THREAD_PRIORITY;
    display_param.sched_priority = DISPLAY_THREAD_PRIORITY;
    timer_param.sched_priority = TIMER_THREAD_PRIORITY;

    pthread_attr_setschedparam(&button_attr, &button_param);
    pthread_attr_setschedparam(&display_attr, &display_param);
    pthread_attr_setschedparam(&timer_attr, &timer_param);

    pthread_attr_setinheritsched(&button_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&display_attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setinheritsched(&timer_attr, PTHREAD_EXPLICIT_SCHED);
    

    // Initialize our mutexes and GPIOs before running our threads.
    pthread_mutex_init(&stopwatch_mutex, NULL);
    gpio_setup();
    
    pthread_create(&button_thread, &button_attr, button_thread_func, NULL);
    pthread_create(&display_thread, &display_attr, display_thread_func, NULL);
    pthread_create(&timer_thread, &timer_attr, timer_thread_func, NULL);

    // Initialize our mutexes and GPIOs before running.
    pthread_mutex_init(&stopwatch_mutex, NULL);

    gpio_setup();
    
    set_gpio_on(RED_LED_PIN);
    set_gpio_off(GREEN_LED_PIN);
    

    signal(SIGINT, (void (*)(int))cleanup);
    
    pthread_join(button_thread, NULL);
    pthread_join(display_thread, NULL);
    pthread_join(timer_thread, NULL);

    cleanup();
    
    return 0;
}