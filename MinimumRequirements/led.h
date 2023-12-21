#ifndef LEDS
#define LEDS
#define BLINK_SLEEP_TIME 300

/*   LEDS   */
#define D1 22
#define D2 21
#define D3 20

/*   PWM    */
#define PWM_FREQ 1000
#define LEVEL 5
#define DIVIDER 125
#define MIN_BRIGHTNESS 0
#define MAX_BRIGHTNESS 1000

void ledsInit();
void pwmInit();
void allLedsOn();
void allLedsOff();
void blink();

#endif