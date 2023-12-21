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
//                      MACROS                     //
/////////////////////////////////////////////////////


/////////////////////////////////////
//      MEMORY ADDRESSES           //
/////////////////////////////////////
const uint16_t deviceStateAddr = I2C_MEM_SIZE - 1;  // 1 byte: 0 or 1
const uint16_t compartmentFinishedAddr = I2C_MEM_SIZE - 2; // 1 byte: 0 or 1
const uint16_t calibCountAddr = MEM_ADDR_START; // 4 bytes
const uint16_t compartmentsMovedAddr = MAX_LOG_SIZE; // 4 bytes

/////////////////////////////////////////////////////
//             FUNCTION DECLARATIONS               //
/////////////////////////////////////////////////////
bool repeatingTimerCallback(struct repeating_timer *t);

/////////////////////////////////////////////////////
//                GLOBAL VARIABLES                 //
/////////////////////////////////////////////////////
static volatile bool sw0_buttonEvent = false;
static volatile bool sw2_buttonEvent = false;

extern int calibration_count;
extern int revolution_counter;
extern bool calibrated;
extern bool pill_detected;
extern bool fallingEdge;

/////////////////////////////////////////////////////
//                 ENUM for STATES                 //
/////////////////////////////////////////////////////
enum SystemState {
    CALIB_WAITING,       // EEPROM, CALIBRATED: 0 == CALIB_WAITING
    DISPENSE_WAITING     //                     1 == DISPENSE_WAITING
};

enum SystemState currentState = CALIB_WAITING;

enum CompartmentState {
    IN_THE_MIDDLE,      // EEPROM, 0 == IN_THE_MIDDLE
    FINISHED            //         1 == FINISHED
};

enum CompartmentState compartmentFinished = IN_THE_MIDDLE;

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

    int compartmentsMoved = 1;

    // check if it was powered off in the middle of motor turning
    bool stored_state = i2cReadByte(deviceStateAddr);
    DBG_PRINT("%d", stored_state);
    if (1 == stored_state) {
        calibrated = true;
        allLedsOff();
        currentState = DISPENSE_WAITING;

        calibration_count = readInt(calibCountAddr);
        DBG_PRINT("Calibration count stored %d\n", calibration_count);
        if (0 < calibration_count) {
            compartmentsMoved = readInt(compartmentsMovedAddr);
            DBG_PRINT("Compartments moved: %d\n", compartmentsMoved);
            if (0 < compartmentsMoved) {
                compartmentFinished = i2cReadByte(compartmentFinishedAddr);
                DBG_PRINT("CompartmentFinished %s", compartmentFinished);
                switch (compartmentFinished) {
                    case IN_THE_MIDDLE:
                        while (false == fallingEdge) {
                            runMotorAntiClockwise(1);
                        }
                        fallingEdge = false;
                        for (int i = 0; i < ALIGNMENT; i++) {
                            runMotorClockwise(1);
                        }
                        for (int i = 0; i < compartmentsMoved; i++) {
                            for (int j = 0; j < (calibration_count / COMPARTMENTS + COMPARTMENTS - 1); j++) {
                                runMotorClockwise(1);
                                if (j == 0) {
                                    compartmentFinished = IN_THE_MIDDLE;
                                    i2cWriteByte(compartmentFinishedAddr, compartmentFinished);
                                }
                            }
                            compartmentFinished = FINISHED;
                            i2cWriteByte(compartmentFinishedAddr, compartmentFinished);
                        }
                        sleep_ms(SLEEP_BETWEEN);
                    case FINISHED:
                        for (; compartmentsMoved <= (COMPARTMENTS - 1); compartmentsMoved++) {
                            for (int i = 0; i < (calibration_count / COMPARTMENTS + COMPARTMENTS - 1); i++) {
                                runMotorClockwise(1);
                                if (i == 0) {
                                    compartmentFinished = IN_THE_MIDDLE;
                                    i2cWriteByte(compartmentFinishedAddr, compartmentFinished);
                                }
                            }
                            if (true == pill_detected) {
                                pill_detected = false;
                                for (int i = 0; i < BLINK_TIMES; i++) {
                                    blink();
                                }
                            }
                            compartmentFinished = FINISHED;
                            i2cWriteByte(compartmentFinishedAddr, compartmentFinished);
                            writeInt(compartmentsMovedAddr, compartmentsMoved);
                            if ((COMPARTMENTS - 1) != compartmentsMoved ) {
                                sleep_ms(SLEEP_BETWEEN / 4);
                            } else {
                                DBG_PRINT("All pills dispensed. Waiting for button press to calibrate.\n");
                            }
                        }
                        currentState = CALIB_WAITING;
                        i2cWriteByte(deviceStateAddr, currentState);
                        compartmentFinished = IN_THE_MIDDLE;
                        i2cWriteByte(compartmentFinishedAddr, compartmentFinished);
                        calibration_count = 0;
                        writeInt(calibCountAddr, calibration_count);
                        compartmentsMoved = 0;
                        writeInt(compartmentsMovedAddr, compartmentsMoved);
                        break;
                }
            }
        }
    }

    while(true) {
        if (true == sw0_buttonEvent) {
            sw0_buttonEvent = false;
            switch (currentState) {
                case CALIB_WAITING:
                    calibrateMotor(); /* calibrates and aligns */
                    allLedsOn();
                    currentState = DISPENSE_WAITING;
                    i2cWriteByte(deviceStateAddr, currentState);
                    writeInt(calibCountAddr, calibration_count);
                    break;
                case DISPENSE_WAITING:
                    break;
            }
        }

        if (true == sw2_buttonEvent) {
            sw2_buttonEvent = false;
            switch (currentState) {
                case CALIB_WAITING:
                    break;
                case DISPENSE_WAITING:
                    allLedsOff();
                    pill_detected = false;
                    /* start dispensing pills */
                    for (; compartmentsMoved <= (COMPARTMENTS - 1); compartmentsMoved++) {
                        for (int i = 0; i < (calibration_count / COMPARTMENTS + COMPARTMENTS - 1); i++) {
                            runMotorClockwise(1);
                            if (i == 0) {
                                compartmentFinished = IN_THE_MIDDLE;
                                i2cWriteByte(compartmentFinishedAddr, compartmentFinished);
                            }
                        }
                        if (true == pill_detected) {
                            pill_detected = false;
                            for (int i = 0; i < BLINK_TIMES; i++) {
                                blink();
                            }
                        }
                        compartmentFinished = FINISHED;
                        i2cWriteByte(compartmentFinishedAddr, compartmentFinished);
                        writeInt(compartmentsMovedAddr, compartmentsMoved);
                        if ((COMPARTMENTS - 1) != compartmentsMoved ) {
                            sleep_ms(SLEEP_BETWEEN / 4);
                        } else {
                            DBG_PRINT("All pills dispensed. Waiting for button press to calibrate.\n");
                        }
                    }
                    currentState = CALIB_WAITING;
                    i2cWriteByte(deviceStateAddr, currentState);
                    compartmentFinished = IN_THE_MIDDLE;
                    i2cWriteByte(compartmentFinishedAddr, compartmentFinished);
                    calibration_count = 0;
                    writeInt(calibCountAddr, calibration_count);
                    compartmentsMoved = 0;
                    writeInt(compartmentsMovedAddr, compartmentsMoved);
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
