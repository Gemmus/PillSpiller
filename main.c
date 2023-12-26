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

#ifndef DEBUG_PRINT
#define DBG_PRINT(f_, ...)  printf((f_), ##__VA_ARGS__)
#else
#define DBG_PRINT(f_, ...)
#endif

#define DEBUG
#ifdef DEBUG
#define COMPARTMENT_TIME  ( SLEEP_BETWEEN / 8 )
#else
#define COMPARTMENT_TIME  ( SLEEP_BETWEEN )
#endif

/////////////////////////////////////////////////////
//             FUNCTION DECLARATIONS               //
/////////////////////////////////////////////////////
bool repeatingTimerCallback(struct repeating_timer *t);
void resetValues();
void dispensePills();
void printBoot();

/////////////////////////////////////////////////////
//                GLOBAL VARIABLES                 //
/////////////////////////////////////////////////////
static volatile bool sw0_buttonEvent = false;
static volatile bool sw2_buttonEvent = false;

static bool lora_connected = false;

static char retval_str[STRLEN];

extern int calibration_count;
extern bool calibrated;
extern bool pill_detected;
extern bool fallingEdge;

static const char *fixed_msg[5] = {"Boot.\n",
                                   "Calibrated. Waiting for button press to dispense pills.\n",
                                   "Powered off during dispense. Motor was not turning.\n",
                                   "Powered off during dispense. Motor was turning.\n",
                                   "All pills dispensed. Waiting for button press to calibrate.\n"};

/////////////////////////////////////////////////////
//                 ENUM for STATES                 //
/////////////////////////////////////////////////////
machineState machine = {
        .currentState = CALIB_WAITING,
        .compartmentFinished = IN_THE_MIDDLE,
        .calibrationCount = 0,
        .compartmentsMoved = 0,
        .logCounter = 0,
};

volatile int *log_counter = &machine.logCounter;

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
#if 0
    /* Initializes lorawan */
    while (!lora_connected) {
        lora_connected = loraInit();
    }
#endif

    struct repeating_timer timer;
    add_repeating_timer_ms(BUTTON_PERIOD, repeatingTimerCallback, NULL, &timer);
    gpio_set_irq_enabled_with_callback(OPTOFORK, GPIO_IRQ_EDGE_FALL, true, gpioFallingEdge);
    gpio_set_irq_enabled(PIEZO, GPIO_IRQ_EDGE_FALL, true);

    if (readStruct(&machine)) {
        if (machine.currentState == DISPENSE_WAITING) {
            calibration_count = machine.calibrationCount;
            printLog(); // for testing purposes, in the final it does not need to be here
            calibrated = true;
            allLedsOff();
            printBoot();

            switch (machine.compartmentFinished) {
                case IN_THE_MIDDLE:
#if 0
                    loraMsg(fixed_msg[3], strlen(fixed_msg[3]), retval_str);
#endif
                    realignMotor();
                    DBG_PRINT("Restored last known state.\n");
                    sleep_ms(COMPARTMENT_TIME);
                    machine.compartmentFinished = FINISHED;
                    writeStruct(&machine);
                    dispensePills();
                    resetValues();
                    machine.currentState = CALIB_WAITING;
                    break;
                case FINISHED:
#if 0
                    loraMsg(fixed_msg[2], strlen(fixed_msg[2]), retval_str);
#endif
                    machine.compartmentsMoved++;
                    sleep_ms(COMPARTMENT_TIME);
                    dispensePills();
                    resetValues();
                    machine.currentState = CALIB_WAITING;
                    break;
            }
        }
    } else {
        printBoot();
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
#if 0
                    loraMsg(fixed_msg[1], strlen(fixed_msg[1]), retval_str);
#endif
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
    char dispensed_msg[STRLEN/2-3];
    bool pill_dispensed = false;

    allLedsOff();
    pill_detected = false;

    /* start dispensing pills */
    for (; machine.compartmentsMoved < COMPARTMENTS; machine.compartmentsMoved++) {

        for (int i = 0; i < (calibration_count / COMPARTMENTS + COMPARTMENTS - 1); i++) {
            runMotorClockwise(1);
            if (i == 0) {
                machine.compartmentFinished = IN_THE_MIDDLE;
                writeStruct(&machine);
            }
            if (i % 4 == 0) {
                i2cWriteByte_NoDelay(STEPPER_POSITION_ADDRESS, i/4);
            }
            if (true == pill_detected) {
                pill_detected = false;
                pill_dispensed = true;
            }
        }

        machine.compartmentFinished = FINISHED;
        writeStruct(&machine);

        if (true == pill_dispensed) {
            /* msg of dispensed pill to eeprom and lorawan */
            sprintf(dispensed_msg, "Day %d: Pill dispensed. Number of pills left: %d.\n", (const char *) machine.compartmentsMoved, COMPARTMENTS - machine.compartmentsMoved - 1);
            DBG_PRINT("%s", dispensed_msg);
            writeLogEntry(dispensed_msg);
            writeStruct(&machine); // Update log counter
#if 0
            loraMsg(dispensed_msg, strlen(dispensed_msg), retval_str);
#endif
            for (int j = 0; j < BLINK_TIMES; j++) {
                blink();
            }
        } else {
            /* msg of not dispensed pill to eeprom and lorawan */
            sprintf(dispensed_msg, "Day %d: Pill not dispensed. Number of pills left: %d.\n", (const char *) machine.compartmentsMoved, COMPARTMENTS - machine.compartmentsMoved - 1);
            DBG_PRINT("%s", dispensed_msg);
            writeLogEntry(dispensed_msg);
            writeStruct(&machine); // Update log counter
#if 0
            loraMsg(dispensed_msg, strlen(dispensed_msg), retval_str);
#endif
        }

        if ((COMPARTMENTS - 1) > machine.compartmentsMoved) {
            if (true == pill_dispensed) {
                pill_dispensed = false;
                sleep_ms(COMPARTMENT_TIME - BLINK_SLEEP_TIME * 2 * BLINK_TIMES);
            } else {
                sleep_ms(COMPARTMENT_TIME);
            }
        } else {
            DBG_PRINT("%s", fixed_msg[4]);
#if 0
            loraMsg(fixed_msg[4], strlen(fixed_msg[4]), retval_str);
#endif
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

void printBoot() {
    DBG_PRINT(fixed_msg[0]); /* MSG */
    writeLogEntry(fixed_msg[0]);
    writeStruct(&machine); // Update log counter
#if 0
    loraMsg(fixed_msg[0], strlen(fixed_msg[0]), retval_str);
#endif
}
