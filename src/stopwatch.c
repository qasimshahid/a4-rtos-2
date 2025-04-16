#include <pthread.h>
#include <sys/time.h>
#include <signal.h>
#include <sched.h>
#include <unistd.h>
#include "bbbio.h"

// Mutex for thread synchronization
static pthread_mutex_t mutex;

// Some protected resources we will use between threads.
static float32_t current_time = 0;      
static int32_t stopwatch_running = 0;  
static int32_t reset_requested = 0;

// Set by asking user for GPIO pins.
static int32_t START_STOP_BUTTON_PIN = -1;
static int32_t RESET_BUTTON_PIN = -1;
static int32_t RED_LED_PIN = -1;
static int32_t GREEN_LED_PIN = -1;

// Thread priorities - check the main function at the bottom of this code. We are dynamically getting min and max.

// Helper function to safely lock
static void lockMutex(void) {

    int32_t ret = pthread_mutex_lock(&mutex);
    if (ret != 0) {
        (void) printf("ERROR: Mutex lock failed! Sending SIGINT...\n");
        (void) raise(SIGINT);  // Simulate pressing CTRL+C
    }
}

// Helper function to safely unlock
static void unlockMutex(void) {

    int32_t ret = pthread_mutex_unlock(&mutex);
    if (ret != 0) {
        (void) printf("ERROR: Mutex unlock failed! Sending SIGINT...\n");
        (void) raise(SIGINT);  // Simulate pressing CTRL+C
    }
}

//Button thread function - Reads button states every 10ms and updates stopwatch state accordingly
static void *button_thread_func(void) {
    int32_t start_stop_prev = 0;
    int32_t start_stop_current = 0;
    int32_t reset_prev = 0;
    int32_t reset_current = 0;
    int32_t state = 0;
    
    while (1 == 1) {
        start_stop_current = read_gpio_value(START_STOP_BUTTON_PIN);
        reset_current = read_gpio_value(RESET_BUTTON_PIN);
        
        if ((int32_t) start_stop_current == 1 && (int32_t) start_stop_prev == 0) {
            lockMutex();
            // Toggle stopwatch state
            stopwatch_running = (!(int32_t)stopwatch_running);
            state = stopwatch_running;
            unlockMutex();
            
            // Update LEDs based on state
            if (state == 1) {
                set_gpio_off(RED_LED_PIN);
                set_gpio_on(GREEN_LED_PIN);
            } else {
                set_gpio_on(RED_LED_PIN);
                set_gpio_off(GREEN_LED_PIN);
            }
        
        }
        // Check for reset button press
        else if ((int32_t) reset_current == 1 && (int32_t) reset_prev == 0) {
            lockMutex();
            reset_requested = 1;
            unlockMutex();
        }
        else {
        }
        
        // Update previous button states
        start_stop_prev = start_stop_current;
        reset_prev = reset_current;

        (void) usleep(10000); // Sleep for 10ms - button polling period of 10 ms. 
        // Every 10 ms, buttons are read with high priority.
    }
    
    return NULL;
}

// Display thread function - Updates the terminal display every 100ms
static void *display_thread_func(void) {
    float32_t time_to_display = 0.0f;
    int32_t is_running = 0;
    
    while (1 == 1) {
        lockMutex();
        time_to_display = current_time;
        is_running = stopwatch_running;
        unlockMutex();

        // Clear the current line
        (void) printf("\r                                        \r");
        
        if (is_running == 1) {
            // Display with 100ms resolution when running
            (void) printf("Time: %.1f seconds", time_to_display);
        } else {
            // Display with 10ms resolution when stopped
            (void) printf("Time: %.2f seconds", time_to_display);
        }
        
        // Ensure output is displayed immediately
        (void) fflush(stdout);
        
        // Sleep for 100ms (display update period)
        (void) usleep(100000);
    }
    
    return NULL;
}

// Timer thread - measures elasped time and sets the counter. 
static void *timer_thread_func(void) {
    struct timespec last_time, current_time_val;
    float32_t elapsed_time;

    // Get initial time using CLOCK_MONOTONIC (Clock that cannot be set and represents monotonic time since some unspecified starting point.)
    // This initial time is what we will use to measure elapsed time by getting the times afterward.
    (void) clock_gettime(CLOCK_MONOTONIC, &last_time);

    while (1 == 1) {
        // Get current time
        (void) clock_gettime(CLOCK_MONOTONIC, &current_time_val);

        // Calculate elapsed time in seconds
        elapsed_time = (current_time_val.tv_sec - last_time.tv_sec) +  // Calculate the difference in seconds
                        (current_time_val.tv_nsec - last_time.tv_nsec) / 1000000000.0f;  // Calculate the difference in nanoseconds and then finally convert to seconds 

        // Update last time for next iteration
        last_time = current_time_val;

        lockMutex();

        // Handle reset if requested
        if (reset_requested == 1) {
            current_time = 0.0f;
            reset_requested = 0;
        } 
        else if (stopwatch_running == 1) { // Update current time if stopwatch is running
            if (current_time + elapsed_time <= (float) FLT_MAX) {
                current_time += elapsed_time;
            }
            else {
                current_time = 0.0f; // Reset to 0 if overflow occurs
            }
        }
        else {
            unlockMutex();
            (void) usleep(10000); // Sleep for 10ms if not running
            continue;
        }

        unlockMutex();
        (void) usleep(10000);
    }

    return NULL;
}

// Set up GPIO pins using the bbbio library. 0 on success, -1 otherwise.
static int32_t get_input_and_initialize_gpio(void) {

    // Initialize as -1 for unsuccessful until we make sure we setup our GPIO pins properly. Successful is ret = 0.
    Buffer input;
    int32_t ret = -1;

    (void) printf("Please provide GPIO pin numbers for the buttons and LEDs. Format:\n");
    (void) printf("Button 1 GPIO Pin (timer stop/start),Button 2 GPIO Pin (Reset),Red LED GPIO Pin,Green LED GPIO Pin\n");

    if (fgets((char*)input, sizeof(input), stdin) != NULL)
    {
        if (sscanf(((char*)(BufferPointer)input), "%d,%d,%d,%d", &START_STOP_BUTTON_PIN, &RESET_BUTTON_PIN, &RED_LED_PIN, &GREEN_LED_PIN) != 4)
        {
            (void) printf("Invalid input format. Please enter four integers separated by commas.\n");
            ret = -1;
        }
        else
        {
            ret = 0;
        }
    }
    else 
    {
        ret = -1;
    }

    if 
    ( 
        ret == 0 && // Make sure previous part was successful.
        setup_gpio_pin(START_STOP_BUTTON_PIN, (BufferPointer) GPIO_INPUT_MODE) && 
        setup_gpio_pin(RESET_BUTTON_PIN, (BufferPointer) GPIO_INPUT_MODE) && 
        setup_gpio_pin(RED_LED_PIN, (BufferPointer)GPIO_OUTPUT_MODE) && 
        setup_gpio_pin(GREEN_LED_PIN, (BufferPointer) GPIO_OUTPUT_MODE)
    ) 
    {
        set_gpio_on(RED_LED_PIN);
        set_gpio_off(GREEN_LED_PIN);
        ret = 0;
    }
    else 
    {
        ret = -1;
    }

    return ret;
}

// Helper function to check for return values and print error messages. Exits on any errors.
static void check(int32_t result, BufferPointer msg) {
    if (result != 0) {
        (void) printf("[ERROR] %s: %s\n", msg, strerror(result));
        (void) exit(1);
    }
}

// Cleanup function to reset GPIO states and destroy mutex
static void cleanup(int32_t signum) {
    lockMutex();
    set_gpio_off(RED_LED_PIN);
    set_gpio_off(GREEN_LED_PIN);

    // Destroy mutex
    (void) pthread_mutex_destroy(&mutex);

    (void) printf("\nStopwatch application terminated.\n");
    exit(0);
}

// Main function that has all our code that runs our threads and handles setting up priorities for them.
int32_t main(void) {

    (void) signal(SIGINT, &cleanup); // CTRL+C
    (void) signal(SIGTSTP, &cleanup); // CTRL+Z
    (void) signal(SIGTERM, &cleanup); // Kill command
    (void) signal(SIGQUIT, &cleanup); // CTRL+ \ /

    // Set up threads with real-time priority using FIFO.
    pthread_t button_thread, display_thread, timer_thread;
    pthread_attr_t button_attr, display_attr, timer_attr;
    struct sched_param button_param, display_param, timer_param;

    // Init attributes
    // Small note - these check functions will exit if we find anything bad going on. 
    check((int32_t) pthread_attr_init(&button_attr), (BufferPointer) "pthread_attr_init (button)");
    check((int32_t) pthread_attr_init(&display_attr), (BufferPointer) "pthread_attr_init (display)");
    check((int32_t) pthread_attr_init(&timer_attr), (BufferPointer) "pthread_attr_init (timer)");

    // Set scheduling policy (FIFO for real-time)
    check((int32_t) pthread_attr_setschedpolicy(&button_attr, SCHED_FIFO), (BufferPointer) "setschedpolicy (button)");
    check((int32_t) pthread_attr_setschedpolicy(&display_attr, SCHED_FIFO), (BufferPointer) "setschedpolicy (display)");
    check((int32_t) pthread_attr_setschedpolicy(&timer_attr, SCHED_FIFO), (BufferPointer) "setschedpolicy (timer)");

    // Set priorities from our macros
    // Get min and max priorities for SCHED_FIFO
    int32_t min_priority = (int32_t) sched_get_priority_min(SCHED_FIFO);
    int32_t max_priority = (int32_t) sched_get_priority_max(SCHED_FIFO);
    if ((int32_t) min_priority == -1 || (int32_t) max_priority == -1) {
        perror("sched_get_priority failed");
        exit(1);
    }

    // Assign priorities based on Rate Monotonic Scheduling
    int32_t button_priority  = max_priority;         // Fastest period (10ms)
    int32_t timer_priority   = max_priority - 10;    // Mid (10ms but maybe not blocking)
    int32_t display_priority = min_priority + 50;         // Slowest (100ms)
    // We are using three threads so we can use max and min priorities. Won't make much difference.

    // Print for verification
    (void) printf("Assigned Priorities:\n");
    (void) printf("  Button  Thread: %d\n", button_priority);
    (void) printf("  Timer   Thread: %d\n", timer_priority);
    (void) printf("  Display Thread: %d\n", display_priority);

    // Set thread priorities
    button_param.sched_priority = button_priority;
    display_param.sched_priority = display_priority;
    timer_param.sched_priority = timer_priority;

    check((int32_t) pthread_attr_setschedparam(&button_attr, &button_param), (BufferPointer) "setschedparam (button)");
    check((int32_t) pthread_attr_setschedparam(&display_attr, &display_param), (BufferPointer) "setschedparam (display)");
    check((int32_t) pthread_attr_setschedparam(&timer_attr, &timer_param), (BufferPointer) "setschedparam (timer)");

    // Use explicit scheduling to make sure the thread runs with the specified priority and not with that of parent.
    check((int32_t) pthread_attr_setinheritsched(&button_attr, PTHREAD_EXPLICIT_SCHED), (BufferPointer) "setinheritsched (button)");
    check((int32_t) pthread_attr_setinheritsched(&display_attr, PTHREAD_EXPLICIT_SCHED), (BufferPointer) "setinheritsched (display)");
    check((int32_t) pthread_attr_setinheritsched(&timer_attr, PTHREAD_EXPLICIT_SCHED), (BufferPointer) "setinheritsched (timer)");

    // Init mutex. Use mutex attributes to configure how the mutex works.
    // Set the protocol for the mutex. We're using "PTHREAD_PRIO_INHERIT" to ensure priority inheritance,
    // so when a lower-priority thread holds the mutex, it temporarily inherits the higher priority of the thread that’s waiting. This is to mitigate any sort of priority inversion. 
    // Documentation (https://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_mutexattr_setprotocol.html):
    //  When a thread is blocking higher priority threads because of owning one or more mutexes with the PTHREAD_PRIO_INHERIT protocol attribute,
    //  it executes at the higher of its priority or the priority of the highest priority thread waiting on any of the mutexes owned by this thread and initialised with this protocol.
    pthread_mutexattr_t mutex_attr;
    check(pthread_mutexattr_init(&mutex_attr), (BufferPointer) "pthread_mutexattr_init");
    check(pthread_mutexattr_setprotocol(&mutex_attr, PTHREAD_PRIO_INHERIT), (BufferPointer) "pthread_mutexattr_setprotocol");
    check(pthread_mutex_init(&mutex, &mutex_attr), (BufferPointer) "pthread_mutex_init");
    
    check((int32_t) get_input_and_initialize_gpio(), (BufferPointer) "gpio_setup");
    
    // Start our threads.
    check((int32_t) pthread_create(&button_thread, &button_attr, &button_thread_func, NULL), (BufferPointer) "pthread_create (button)");
    check((int32_t) pthread_create(&display_thread, &display_attr, &display_thread_func, NULL), (BufferPointer) "pthread_create (display)");
    check((int32_t) pthread_create(&timer_thread, &timer_attr, &timer_thread_func, NULL), (BufferPointer) "pthread_create (timer)");
    
    // We will never reach here since we have threads with inifinte loops.
    (void) pthread_join(button_thread, NULL);
    (void) pthread_join(display_thread, NULL);
    (void) pthread_join(timer_thread, NULL);
    
    return 0;
}