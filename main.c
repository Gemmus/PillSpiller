#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"

#include "led.h"
#include "button.h"
#include "uart.h"
#include "lorawan.h"
#include "eeprom.h"
#include "steppermotor.h" // includes stepper motor, optofork and piezo related codes

#ifdef DEBUG_PRINT
#define DBG_PRINT(f_, ...)  printf((f_), ##__VA_ARGS__)
#else
#define DBG_PRINT(f_, ...)
#endif

#define DEBUG
#ifndef DEBUG
#define COMPARTMENT_TIME  ( SLEEP_BETWEEN / 8 )
#else
#define COMPARTMENT_TIME  ( SLEEP_BETWEEN )
#endif

#define LORAWAN_CONN

#ifdef LORAWAN_CONN
#define LORAWAN_COMM_TIME  ( MSG_WAITING_TIME )
#else
#define LORAWAN_COMM_TIME  ( 0 )
#endif

/////////////////////////////////////////////////////
//             FUNCTION DECLARATIONS               //
/////////////////////////////////////////////////////

bool repeatingTimerCallback(struct repeating_timer *t);
bool blinkTimerCallback(struct repeating_timer *t);
void resetValues();
void dispensePills();
void eepromLorawanComm(const char* message, size_t msg_size);
void noDetectBlink();

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

static const char *fixed_msg[7] = {"Boot.",
                                   "Calibrated. Waiting for button to dispense pills.",
                                   "Powered off during dispense. Motor was not turning.",
                                   "Powered off during dispense. Motor was turning.",
                                   "All pills dispensed. Waiting for button to calibrate.",
                                   "Booted after calibration. Waiting for button to dispense.",
                                   "Waiting for button to calibrate."};

/////////////////////////////////////////////////////
//                     STRUCT                      //
/////////////////////////////////////////////////////

machineState machine = {
        .currentState = CALIB_WAITING,
        .compartmentFinished = IN_THE_MIDDLE,
        .calibrationCount = 0,
        .compartmentsMoved = 0,
        .logCounter = 0,
};

volatile int *log_counter = &machine.logCounter;

static struct repeating_timer blink_timer;
static uint32_t blink_counter;

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

    //eraseAll(); /* Deletes all data from eeprom from log area */

#ifdef LORAWAN_CONN
    /* Initializes lorawan */
    /*
    while (!lora_connected) {
        lora_connected = loraInit();
    }*/
#endif

#if 0
    /* to set the uart TIMEOUT value: */
    if (true == loraCommunication("AT+UART=TIMEOUT,0\r\n", STD_WAITING_TIME, retval_str)) {
        printf("%s\n",retval_str);
    }
#endif

    struct repeating_timer button_timer;
    add_repeating_timer_ms(BUTTON_PERIOD, repeatingTimerCallback, NULL, &button_timer);
    gpio_set_irq_enabled_with_callback(OPTOFORK, GPIO_IRQ_EDGE_FALL, true, gpioFallingEdge);
    gpio_set_irq_enabled(PIEZO, GPIO_IRQ_EDGE_FALL, true);

    if (readStruct(&machine)) {
        if (machine.currentState == CALIB_WAITING) {
            eepromLorawanComm(fixed_msg[0], strlen(fixed_msg[0]));
            eepromLorawanComm(fixed_msg[6], strlen(fixed_msg[6]));
        }
        if (machine.currentState == DISPENSE_WAITING) {
            calibration_count = machine.calibrationCount;
            printLog();
            calibrated = true;
            allLedsOff();
            eepromLorawanComm(fixed_msg[0], strlen(fixed_msg[0]));

            switch (machine.compartmentFinished) {
                case IN_THE_MIDDLE:

                    if (0 != machine.compartmentsMoved) {
                        eepromLorawanComm(fixed_msg[3], strlen(fixed_msg[3]));
                    }

                    realignMotor();
                    sleep_ms(COMPARTMENT_TIME);
                    machine.compartmentFinished = FINISHED;
                    writeStruct(&machine);
                    dispensePills();
                    printLog();
                    resetValues();
                    machine.currentState = CALIB_WAITING;
                    break;
                case FINISHED:
                    if (0 == machine.compartmentsMoved) {
                        eepromLorawanComm(fixed_msg[5], strlen(fixed_msg[5]));
                        machine.compartmentsMoved = 1;
                        allLedsOn();
                        break;
                    } else {
                        machine.compartmentsMoved++;
                        eepromLorawanComm(fixed_msg[2], strlen(fixed_msg[2]));
                        sleep_ms(COMPARTMENT_TIME);
                        dispensePills();
                        printLog();
                        resetValues();
                        machine.currentState = CALIB_WAITING;
                        break;
                    }
            }
        }
    } else {
        eepromLorawanComm(fixed_msg[0], strlen(fixed_msg[0]));
        eepromLorawanComm(fixed_msg[6], strlen(fixed_msg[6]));
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
                    machine.compartmentFinished = 1;
                    eepromLorawanComm(fixed_msg[1], strlen(fixed_msg[1]));
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
                    machine.compartmentsMoved = 1;
                    dispensePills();
                    printLog();
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

/////////////////////////////////////////////////////
//                   FUNCTIONS                     //
/////////////////////////////////////////////////////

/**********************************************************************************************************************
 * \brief: Function called by timer to check if SW_0 or SW_2 buttons were pressed. Filtered button press sets the
 *         button event flag respectively.
 *
 * \param: struct repeating_timer *t, not used.
 *
 * \return: boolean, returns true everytime function is called.
 *
 * \remarks:
 **********************************************************************************************************************/
bool repeatingTimerCallback(struct repeating_timer *t) {
    /* SW0 */
    static uint sw0_button_state = 0, sw0_filter_counter = 0;
    uint sw0_new_state = gpio_get(SW_0);
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
    uint sw2_new_state = gpio_get(SW_2);
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

/**********************************************************************************************************************
 * \brief: Function called by timer to blink 5 times if pill not detected from compartment.
 *
 * \param: struct repeating_timer *t, not used.
 *
 * \return: boolean, returns true if blink_counter is greater than zero, else returns false.
 *
 * \remarks:
 **********************************************************************************************************************/
bool blinkTimerCallback(struct repeating_timer *t) {
    if (blink_counter % 2) {
        allLedsOff();
    } else {
        allLedsOn();
    }
    blink_counter--;

    if (blink_counter) {
        return true;
    } else {
        return false;
    }
}

/**********************************************************************************************************************
 * \brief: Dispenses 7 pills resided in 7 different compartments using stepper motor. Controls led lights and blinking
 *         according to events. During the process the function also updates the states and counters, and creates log
 *         messages respectively that are transmitted both to EEPROM and via LoRaWAN to network.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void dispensePills() {
    char dispensed_msg[STRLEN/2-3];
    bool pill_dispensed = false;

    allLedsOff();

    /* start dispensing pills */
    for (; machine.compartmentsMoved < COMPARTMENTS; machine.compartmentsMoved++) {

        pill_detected = false;
        pill_dispensed = false;

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
            sprintf(dispensed_msg, "Day %d: Pill dispensed. Number of pills left: %d.", (const char *) machine.compartmentsMoved, COMPARTMENTS - machine.compartmentsMoved - 1);
            eepromLorawanComm(dispensed_msg, strlen(dispensed_msg));
        } else {
            noDetectBlink();
            sprintf(dispensed_msg, "Day %d: Pill not dispensed. Number of pills left: %d.", (const char *) machine.compartmentsMoved, COMPARTMENTS - machine.compartmentsMoved - 1);
            eepromLorawanComm(dispensed_msg, strlen(dispensed_msg));
        }

        if ((COMPARTMENTS - 1) > machine.compartmentsMoved) {
            sleep_ms(COMPARTMENT_TIME - I2C_MEM_WRITE_TIME - LORAWAN_COMM_TIME);
        } else {
            eepromLorawanComm(fixed_msg[4], strlen(fixed_msg[4]));
        }
    }
}

/**********************************************************************************************************************
 * \brief: Resets the variables of the struct to their initial states and updates these values to EEPROM.
 *
 * \param:
 *
 * \return:
 *
 * \remarks: machine.logCounter is not reset to persist the existing log messages.
 **********************************************************************************************************************/
void resetValues() {
    machine.currentState = CALIB_WAITING;
    machine.compartmentFinished = IN_THE_MIDDLE;
    machine.calibrationCount = 0;
    machine.compartmentsMoved = 0;
    machine.logCounter = 0;
    writeStruct(&machine);
}

/**********************************************************************************************************************
 * \brief: Transmits passed message to EEPROM as a log message and also via LoRaWAN to the network. Updates the struct
 *         to EEPROM.
 *
 * \param: 2 params: pointer to a const char message and its length as size_t type.
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void eepromLorawanComm(const char* message, size_t msg_size) {
    DBG_PRINT("%s\n", message);
    writeLogEntry(message);
    writeStruct(&machine);
#ifdef LORAWAN_CONN
    loraMsg(message, msg_size, retval_str);
#endif
}

/**********************************************************************************************************************
 * \brief: Activates the timer to no pill detection blinking.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void noDetectBlink() {
    blink_counter = BLINK_TIMES * 2 - 1;
    add_repeating_timer_ms(BLINK_SLEEP_TIME, blinkTimerCallback, NULL, &blink_timer);
    allLedsOn();
}
