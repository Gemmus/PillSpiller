#include "pico/stdlib.h"
#include "steppermotor.h"
#include "led.h"
#include <stdio.h>

static const uint turning_sequence[8][4] = {{1, 0, 0, 0},
                                            {1, 1, 0, 0},
                                            {0, 1, 0, 0},
                                            {0, 1, 1, 0},
                                            {0, 0, 1, 0},
                                            {0, 0, 1, 1},
                                            {0, 0, 0, 1},
                                            {1, 0, 0, 1}};

volatile bool calibrated = false;
volatile bool fallingEdge = false;
static volatile int revolution_counter = 0;
volatile int calibration_count = 0;
static volatile int row = 0;
static const int stepper_array[] = {IN1, IN2, IN3, IN4};
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
    printf("Number of steps per revolution: %u\n", calibration_count);
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
        gpio_put(IN4, turning_sequence[row][3]);
        gpio_put(IN3, turning_sequence[row][2]);
        gpio_put(IN2, turning_sequence[row][1]);
        gpio_put(IN1, turning_sequence[row][0]);
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