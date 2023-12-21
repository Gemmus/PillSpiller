#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

#include "led.h"
#include "button.h"
#include "uart.h"
#include "lorawan.h"
#include "eeprom.h"
#include "steppermotor.h" // includes stepper motor, optofork and piezo related codes

/////////////////////////////////////////////////////
//                      MACROS                     //
/////////////////////////////////////////////////////


/////////////////////////////////////////////////////
//             FUNCTION DECLARATIONS               //
/////////////////////////////////////////////////////
bool repeatingTimerCallback(struct repeating_timer *t);

/////////////////////////////////////////////////////
//                GLOBAL VARIABLES                 //
/////////////////////////////////////////////////////
static volatile bool sw0_buttonEvent = false;
static volatile bool sw1_buttonEvent = false;

extern bool calibrated;
extern int calibration_count;
extern bool pill_detected;

/////////////////////////////////////////////////////
//                 ENUM for STATES                 //
/////////////////////////////////////////////////////
enum SystemState {
    CALIB_WAITING,
    DISPENSE_WAITING
};

enum SystemState currentState = CALIB_WAITING;

/////////////////////////////////////////////////////
//                     MAIN                        //
/////////////////////////////////////////////////////
int main(void) {

    stdio_init_all();
    ledsInit();
    buttonsInit();
    pwmInit();
    stepperMotorInit();
    optoforkInit();
    piezoInit();

    struct repeating_timer timer;
    add_repeating_timer_ms(BUTTON_PERIOD, repeatingTimerCallback, NULL, &timer);
    gpio_set_irq_enabled_with_callback(OPTOFORK, GPIO_IRQ_EDGE_FALL, true, gpioFallingEdge);
    gpio_set_irq_enabled(PIEZO, GPIO_IRQ_EDGE_FALL, true);

    while(true) {
        if (true == sw0_buttonEvent) {
            sw0_buttonEvent = false;
            switch (currentState) {
                case CALIB_WAITING:
                    calibrateMotor(); // calibrate and align
                    allLedsOn();
                    currentState = DISPENSE_WAITING;
                    break;
                case DISPENSE_WAITING:
                    break;
            }
        }

        if (true == sw1_buttonEvent) {
            sw1_buttonEvent = false;
            switch (currentState) {
                case CALIB_WAITING:
                    break;
                case DISPENSE_WAITING:
                    allLedsOff();
                    // start dispensing pills
                    for (int i = 0; i < (COMPARTMENTS - 1); i++) {
                        runMotorClockwise(calibration_count / COMPARTMENTS + COMPARTMENTS - 1);
                        if (true == pill_detected) {
                            pill_detected = false;
                            for (int j = 0; j < BLINK_TIMES; j++) {
                                blink();
                            }
                        }
                        sleep_ms(SLEEPTIME_BETWEEN / 2);
                    }
                    currentState = CALIB_WAITING;
                    break;
            }
        }

        if (CALIB_WAITING == currentState) {
            blink();
        }
    }
    return 0;
}

bool repeatingTimerCallback(struct repeating_timer *t) {
    /* SW0 */
    static uint sw0_button_state = 0, sw0_filter_counter = 0;
    uint sw0_new_state = 1;
    sw0_new_state = gpio_get(SW_0);
    if (sw0_button_state != sw0_new_state) {
        if (++sw0_filter_counter >= BUTTON_FILTER) {
            sw0_button_state = sw0_new_state;
            sw0_filter_counter = 0;
            if (sw0_new_state != SW0_RELEASED) {
                sw0_buttonEvent = true;
            }
        }
    } else {
        sw0_filter_counter = 0;
    }
    /* SW1 */
    static uint sw1_button_state = 0, sw1_filter_counter = 0;
    uint sw1_new_state = 1;
    sw1_new_state = gpio_get(SW_1);
    if (sw1_button_state != sw1_new_state) {
        if (++sw1_filter_counter >= BUTTON_FILTER) {
            sw1_button_state = sw1_new_state;
            sw1_filter_counter = 0;
            if (sw1_new_state != SW1_RELEASED) {
                sw1_buttonEvent = true;
            }
        }
    } else {
        sw1_filter_counter = 0;
    }
    return true;
}
