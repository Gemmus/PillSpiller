#ifndef STEPPER_MOTOR
#define STEPPER_MOTOR

/*  STEP MOTOR  */
#define IN1 13
#define IN2 6
#define IN3 3
#define IN4 2
#define ALIGNMENT 380
#define COMPARTMENTS 8
#define SLEEPTIME_BETWEEN 30000

/*  OPTOFORK  */
#define OPTOFORK 28

/*  PIEZO  */
#define PILL_DROP_MS 100
#define BLINK_TIMES 5
#define PIEZO 27

void stepperMotorInit();
void calibrateMotor();
void runMotorAntiClockwise(int times);
void runMotorClockwise(int times);
void optoforkInit();
void optoFallingEdge();
void piezoInit();
void piezoFallingEdge();
void gpioFallingEdge(uint gpio, uint32_t event_mask);

#endif