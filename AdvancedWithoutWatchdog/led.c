#include "led.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

//////////////////////////////////////////////////
//              GLOBAL VARIABLES                //
//////////////////////////////////////////////////

static const uint brightness = MAX_BRIGHTNESS / 20;
static const int led_arr[] = {D1, D2, D3};

//////////////////////////////////////////////////
//                LED FUNCTIONS                 //
//////////////////////////////////////////////////

/**********************************************************************************************************************
 * \brief: Initialises D1, D2, D3 led lights.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void ledsInit() {
    for (int i = 0; i < sizeof(led_arr)/ sizeof(led_arr[0]); i++) {
        gpio_init(led_arr[i]);
        gpio_set_dir(led_arr[i], GPIO_OUT);
    }
}

/**********************************************************************************************************************
 * \brief: Initialises pulse-width modulation for D1, D2 and D3 led lights. Turns off all the led lights at the end of
 *         the code. / Changes the brightness of all the led lights to 0.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void pwmInit() {
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&config, DIVIDER);
    pwm_config_set_wrap(&config, PWM_FREQ - 1);

    // D1: 2A, D2: 2B, D3: 3A
    for (int i = 0; i < sizeof(led_arr)/ sizeof(led_arr[0]); i++) {
        uint dX_slice = pwm_gpio_to_slice_num(led_arr[i]);
        uint dX_chanel = pwm_gpio_to_channel(led_arr[i]);
        pwm_set_enabled(dX_slice, false);
        pwm_init(dX_slice, &config, false);
        pwm_set_chan_level(dX_slice, dX_chanel, LEVEL + 1);
        gpio_set_function(led_arr[i], GPIO_FUNC_PWM);
        pwm_set_enabled(dX_slice, true);
    }
    allLedsOff();
}

/**********************************************************************************************************************
 * \brief: Turns all the led lights on. / Changes the brightness of all the led lights to a set value larger than zero.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void allLedsOn() {
    for (int i = 0; i < sizeof(led_arr)/ sizeof(led_arr[0]); i++) {
        pwm_set_gpio_level(led_arr[i], brightness);
    }
}

/**********************************************************************************************************************
 * \brief: Turns all the led light off. / Changes the brightness of all the led lights to 0.
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void allLedsOff() {
    for (int i = 0; i < sizeof(led_arr)/ sizeof(led_arr[0]); i++) {
        pwm_set_gpio_level(led_arr[i], MIN_BRIGHTNESS);
    }
}

/**********************************************************************************************************************
 * \brief: Blinks the led lights once, which takes 600 ms (BLINK_SLEEP_TIME  is 300 ms).
 *
 * \param:
 *
 * \return:
 *
 * \remarks:
 **********************************************************************************************************************/
void blink() {
    allLedsOn();
    sleep_ms(BLINK_SLEEP_TIME);
    allLedsOff();
    sleep_ms(BLINK_SLEEP_TIME);
}