#ifndef STEPPER_MOTOR
#define STEPPER_MOTOR

/*  STEP MOTOR  */
#define IN1 13
#define IN2 6
#define IN3 3
#define IN4 2
#define ALIGNMENT 424
#define COMPARTMENTS 8

/*  OPTOFORK  */
#define OPTOFORK 28

void stepperMotorInit();
void calibrateMotor();
void runMotorAntiClockwise(int times);
void runMotorClockwise(int times);
void optoforkInit();
void optoFallingEdge();

#endif