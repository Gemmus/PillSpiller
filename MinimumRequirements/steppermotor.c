#include "pico/stdlib.h"
#include "steppermotor.h"
#include "eeprom.h"
#include <stdio.h>

#ifndef DEBUG_PRINT
#define DBG_PRINT(f_, ...)  printf((f_), ##__VA_ARGS__)
#else
#define DBG_PRINT(f_, ...)
#endif

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
static volatile int revolution_counter = 0;
volatile int calibration_count = 0;
volatile bool calibrated = false;
volatile bool fallingEdge = false;
volatile bool pill_detected = false;

void stepperMotorInit() {
    for (int i = 0; i < sizeof(stepper_array) / sizeof(stepper_array[0]); i++) {
        gpio_init(stepper_array[i]);
        gpio_set_dir(stepper_array[i], GPIO_OUT);
    }
}

void calibrateMotor() {
    calibrated = false;
    fallingEdge = false;
    while (false == fallingEdge) {
        //runMotorAntiClockwise(1);
        runMotorClockwise(1);
    }
    fallingEdge = false;
    while (false == fallingEdge) {
        //runMotorAntiClockwise(1);
        runMotorClockwise(1);
    }
    calibrated = true;
    DBG_PRINT("Number of steps per revolution: %u\n", calibration_count);
    // TO DO: needs to send calibration count address the value
    runMotorAntiClockwise(ALIGNMENT);
}

void runMotorAntiClockwise(int times) {
    for(int i  = 0; i < times; i++) {
        for (int j = 0; j < sizeof(stepper_array) / sizeof(stepper_array[0]); j++) {
            gpio_put(stepper_array[j], turning_sequence[row][j]);
        }
        if (++row >= 8) {
            row = 0;
        }
        sleep_ms(4);
    }
}

void runMotorClockwise(int times) {
    for(; times > 0; times--) {
        for (int j = 0; j < sizeof(stepper_array) / sizeof(stepper_array[0]); j++) {
            gpio_put(stepper_array[j], turning_sequence[row][j]);
        }
        revolution_counter++;
        if (--row <= -1) {
            row = 7;
        }
        sleep_ms(4);
    }
}

void optoforkInit() {
    gpio_init(OPTOFORK);
    gpio_set_dir(OPTOFORK, GPIO_IN);
    gpio_pull_up(OPTOFORK);
}

void optoFallingEdge() {
    fallingEdge = true;
    if (false == calibrated) {
        calibration_count = revolution_counter;
    }
    revolution_counter = 0;
}

void piezoInit() {
    gpio_init(PIEZO);
    gpio_set_dir(PIEZO, GPIO_IN);
    gpio_pull_up(PIEZO);
}

void piezoFallingEdge() {
    pill_detected = true;
}

void gpioFallingEdge(uint gpio, uint32_t event_mask) {
    if (OPTOFORK == gpio) {
        optoFallingEdge();
    } else {
        piezoFallingEdge();
    }
}
