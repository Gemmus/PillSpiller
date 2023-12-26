#include <stdio.h>
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

#ifndef DEBUG_PRINT
#define DBG_PRINT(f_, ...)  printf((f_), ##__VA_ARGS__)
#else
#define DBG_PRINT(f_, ...)
#endif

/////////////////////////////////////////////////////
//             FUNCTION DECLARATIONS               //
/////////////////////////////////////////////////////
bool repeatingTimerCallback(struct repeating_timer *t);
void resetValues();
void dispensePills();

/////////////////////////////////////////////////////
//                GLOBAL VARIABLES                 //
/////////////////////////////////////////////////////
static volatile bool sw0_buttonEvent = false;
static volatile bool sw2_buttonEvent = false;

extern int calibration_count;
extern bool calibrated;
extern bool pill_detected;
extern bool fallingEdge;

/////////////////////////////////////////////////////
//                 ENUM for STATES                 //
/////////////////////////////////////////////////////
machineState machine = {
        .currentState = CALIB_WAITING,
        .compartmentFinished = IN_THE_MIDDLE,
        .calibrationCount = 0,
        .compartmentsMoved = 0,
};

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
    i2cInit();

    //eraseAll();

    struct repeating_timer timer;
    add_repeating_timer_ms(BUTTON_PERIOD, repeatingTimerCallback, NULL, &timer);
    gpio_set_irq_enabled_with_callback(OPTOFORK, GPIO_IRQ_EDGE_FALL, true, gpioFallingEdge);
    gpio_set_irq_enabled(PIEZO, GPIO_IRQ_EDGE_FALL, true);

    DBG_PRINT("Boot\n"); /* MSG */

    if (readStruct(&machine)) {
        if (machine.currentState == DISPENSE_WAITING) {
            calibration_count = machine.calibrationCount;
            calibrated = true;
            allLedsOff();

            switch (machine.compartmentFinished) {
                case IN_THE_MIDDLE:
                    realignMotor();
                    sleep_ms(SLEEP_BETWEEN / 8);
                    machine.compartmentFinished = FINISHED;
                    writeStruct(&machine);
                    dispensePills();
                    resetValues();
                    machine.currentState = CALIB_WAITING;
                    break;
                case FINISHED:
                    machine.compartmentsMoved++;
                    sleep_ms(SLEEP_BETWEEN / 8);
                    dispensePills();
                    resetValues();
                    machine.currentState = CALIB_WAITING;
                    break;
            }
        }
    }

    while(true) {
        if (true == sw0_buttonEvent) {
            sw0_buttonEvent = false;
            switch (machine.currentState) {
                case CALIB_WAITING:
                    calibrateMotor(); /* calibrates and aligns */
                    allLedsOn();
                    machine.currentState = DISPENSE_WAITING;
                    machine.calibrationCount = calibration_count;
                    writeStruct(&machine);
                    machine.compartmentsMoved = 1;
                    //DBG_PRINT("Sent currentStat %d\n", currentState);
                    break;
                case DISPENSE_WAITING:
                    break;
            }
        }

        if (true == sw2_buttonEvent) {
            sw2_buttonEvent = false;
            switch (machine.currentState) {
                case CALIB_WAITING:
                    break;
                case DISPENSE_WAITING:
                    dispensePills();
                    resetValues();
                    break;
            }
        }

        if (CALIB_WAITING == machine.currentState) {
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
    /* SW2 */
    static uint sw2_button_state = 0, sw2_filter_counter = 0;
    uint sw2_new_state = 1;
    sw2_new_state = gpio_get(SW_2);
    if (sw2_button_state != sw2_new_state) {
        if (++sw2_filter_counter >= BUTTON_FILTER) {
            sw2_button_state = sw2_new_state;
            sw2_filter_counter = 0;
            if (sw2_new_state != SW2_RELEASED) {
                sw2_buttonEvent = true;
            }
        }
    } else {
        sw2_filter_counter = 0;
    }
    return true;
}

void dispensePills() {
    allLedsOff();
    pill_detected = false;
    /* start dispensing pills */
    for (; machine.compartmentsMoved < COMPARTMENTS; machine.compartmentsMoved++) {
        DBG_PRINT("Compartments moved %d\n", machine.compartmentsMoved);
        for (int i = 0; i < (calibration_count / COMPARTMENTS + COMPARTMENTS - 1); i++) {
            runMotorClockwise(1);
            if (i == 0) {
                machine.compartmentFinished = IN_THE_MIDDLE;
                writeStruct(&machine);
            }
            if (i % 4 == 0) {
                i2cWriteByte(I2C_MEM_SIZE/2, i/4);
            }
            if (true == pill_detected) {
                pill_detected = false;
                for (int j = 0; j < BLINK_TIMES; j++) {
                    blink();
                }
            }
        }
        machine.compartmentFinished = FINISHED;
        writeStruct(&machine);
        if ((COMPARTMENTS - 1) > machine.compartmentsMoved) {
            sleep_ms(SLEEP_BETWEEN / 8);
        } else {
            DBG_PRINT("All pills dispensed. Waiting for button press to calibrate.\n");
        }
    }
}

void resetValues() {
    machine.currentState = CALIB_WAITING;
    machine.compartmentFinished = IN_THE_MIDDLE;
    machine.calibrationCount = 0;
    machine.compartmentsMoved = 0;
    writeStruct(&machine);
}
