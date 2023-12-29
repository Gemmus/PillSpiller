#include "pico/stdlib.h"
#include "steppermotor.h"
#include <stdio.h>
#include "eeprom.h"

#ifndef DEBUG_PRINT
#define DBG_PRINT(f_, ...)  printf((f_), ##__VA_ARGS__)
#else
#define DBG_PRINT(f_, ...)
#endif

//////////////////////////////////////////////////
//              GLOBAL VARIABLES                //
//////////////////////////////////////////////////

static const int stepper_array[] = {IN1, IN2, IN3, IN4};
static const uint turning_sequence[8][4] = {{1, 0, 0, 0},
                                            {1, 1, 0, 0},
                                            {0, 1, 0, 0},
                                            {0, 1, 1, 0},
                                            {0, 0, 1, 0},
                                            {0, 0, 1, 1},
                                            {0, 0, 0, 1},
                                            {1, 0, 0, 1}};

static volatile int row = 0;

volatile int calibration_count;
volatile int revolution_counter = 0;
volatile int calibration_count = 0;
volatile bool calibrated = false;
volatile bool fallingEdge = false;
volatile bool pill_detected = false;

////////////////////////////////////////////////////////////////////////
//       STEPPER MOTOR,  OPTOFORK  AND  PIEZO SENSOR  FUNCTIONS       //
////////////////////////////////////////////////////////////////////////

/**********************************************************************************************************************
 * \brief: Initializes stepper motor.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void stepperMotorInit() {
    for (int i = 0; i < sizeof(stepper_array) / sizeof(stepper_array[0]); i++) {
        gpio_init(stepper_array[i]);
        gpio_set_dir(stepper_array[i], GPIO_OUT);
    }
}

/**********************************************************************************************************************
 * \brief: Calibrates motor by rotating the stepper motor and counting the number opf steps between two falling edge
 *         of the optofork. In the end, the motor aligns dispenser wheel to match the slot of pill drop area.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void calibrateMotor() {
    calibrated = false;
    fallingEdge = false;
    while (false == fallingEdge) {
        runMotorClockwise(1);
    }
    fallingEdge = false;
    while (false == fallingEdge) {
        runMotorClockwise(1);
    }
    calibrated = true;
    DBG_PRINT("Number of steps per revolution: %u\n", calibration_count);
    runMotorAntiClockwise(ALIGNMENT);
}

/**********************************************************************************************************************
 * \brief: Rotates stepper motor anticlockwise by the number of integer passed as parameter.
 *
 * \param: integer
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void runMotorAntiClockwise(int times) {
    for(int i  = 0; i < times; i++) {
        for (int j = 0; j < sizeof(stepper_array) / sizeof(stepper_array[0]); j++) {
            gpio_put(stepper_array[j], turning_sequence[row][j]);
        }
        if (++row >= 8) {
            row = 0;
        }
        sleep_ms(2);
    }
}

/**********************************************************************************************************************
 * \brief: Rotates stepper motor clockwise by the number of integer passed as parameter.
 *
 * \param: integer
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void runMotorClockwise(int times) {
    for(; times > 0; times--) {
        for (int j = 0; j < sizeof(stepper_array) / sizeof(stepper_array[0]); j++) {
            gpio_put(stepper_array[j], turning_sequence[row][j]);
        }
        revolution_counter++;
        if (--row <= -1) {
            row = 7;
        }
        sleep_ms(2);
    }
}

/**********************************************************************************************************************
 * \brief: If reboot occurs during motor turn, realigns motor back to last stored position.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void realignMotor() {
    int stored_position = i2cReadByte(STEPPER_POSITION_ADDRESS) * 4;
    while (0 != stored_position--) {
        runMotorAntiClockwise(1);
    }
}

/**********************************************************************************************************************
 * \brief: Initializes the optofork.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void optoforkInit() {
    gpio_init(OPTOFORK);
    gpio_set_dir(OPTOFORK, GPIO_IN);
    gpio_pull_up(OPTOFORK);
}

/**********************************************************************************************************************
 * \brief: In case of optofork falling edge, fallingEdge flag is set to true. Sets the calibration_count and resets the
 *         revolution_counter to zero.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void optoFallingEdge() {
    fallingEdge = true;
    if (false == calibrated) {
        calibration_count = revolution_counter;
    }
    revolution_counter = 0;
}

/**********************************************************************************************************************
 * \brief: Initializes the piezo sensor.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void piezoInit() {
    gpio_init(PIEZO);
    gpio_set_dir(PIEZO, GPIO_IN);
    gpio_pull_up(PIEZO);
}

/**********************************************************************************************************************
 * \brief: In case of piezo sensor falling edge, pill_detected flag is set to true.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void piezoFallingEdge() {
    pill_detected = true;
}

/**********************************************************************************************************************
 * \brief: In case of any gpio falling edge, function is called to distinguish the gpio and calls the correct function
 *         respectively.
 *
 * \param: uint, represents gpio where the falling occurs. uint32_t event_mask, not used.
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void gpioFallingEdge(uint gpio, uint32_t event_mask) {
    if (OPTOFORK == gpio) {
        optoFallingEdge();
    } else {
        piezoFallingEdge();
    }
}
