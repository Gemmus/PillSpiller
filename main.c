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
#include "piezo.h"
#include "steppermotor.h" // includes stepper motor and optofork related codes

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
volatile bool sw0_buttonEvent = false;
volatile bool sw1_buttonEvent = false;
volatile bool sw2_buttonEvent = false;

// States:
static volatile bool initWaitingState = true;
static volatile bool dispenseWaitingState = false;

extern bool calibrated;
extern int current_position;
extern int alignment_unit;
extern int calibration_count;


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

    struct repeating_timer timer;
    add_repeating_timer_ms(BUTTON_PERIOD, repeatingTimerCallback, NULL, &timer);
    gpio_set_irq_enabled_with_callback(OPTOFORK, GPIO_IRQ_EDGE_FALL, true, optoFallingEdge);

    while(true) {
        if (true == sw0_buttonEvent) {
            sw0_buttonEvent = false;
            if (true == initWaitingState) {
                // calibrate and align
                calibrateMotor();
                current_position = 0;
                // others
                allLedsOn();
                initWaitingState = false;
                dispenseWaitingState = true;
            } else if (true == dispenseWaitingState) {
                // start dispensing things
                for (int i = 0; i < COMPARTMENTS; i++) {
                    runMotorClockwise(calibration_count / COMPARTMENTS);
                    sleep_ms(30000);
                }
            }
        }

        if (true == initWaitingState) {
            allLedsOn();
            sleep_ms(500);
            allLedsOff();
            sleep_ms(500);
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

    return true;
}
