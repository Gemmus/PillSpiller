#include "led.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

static const uint brightness = MAX_BRIGHTNESS / 10;
static const int led_arr[] = {D1, D2, D3};

void ledsInit() {
    for (int i = 0; i < sizeof(led_arr)/ sizeof(led_arr[0]); i++) {
        gpio_init(led_arr[i]);
        gpio_set_dir(led_arr[i], GPIO_OUT);
    }
}

void pwmInit() {
    pwm_config config = pwm_get_default_config();

    // D1: 2A, D2: 2B, D3: 3A
    for (int i = 0; i < sizeof(led_arr)/ sizeof(led_arr[0]); i++) {
        uint dX_slice = pwm_gpio_to_slice_num(led_arr[i]);
        uint dX_chanel = pwm_gpio_to_channel(led_arr[i]);
        pwm_set_enabled(dX_slice, false);
        pwm_config_set_clkdiv_int(&config, DIVIDER);
        pwm_config_set_wrap(&config, PWM_FREQ - 1);
        pwm_init(dX_slice, &config, false);
        pwm_set_chan_level(dX_slice, dX_chanel, LEVEL + 1);
        gpio_set_function(led_arr[i], GPIO_FUNC_PWM);
        pwm_set_enabled(dX_slice, true);
    }
    allLedsOff();
}

void allLedsOn() {
    for (int i = 0; i < sizeof(led_arr)/ sizeof(led_arr[0]); i++) {
        pwm_set_gpio_level(led_arr[i], brightness);
    }
}

void allLedsOff() {
    for (int i = 0; i < sizeof(led_arr)/ sizeof(led_arr[0]); i++) {
        pwm_set_gpio_level(led_arr[i], MIN_BRIGHTNESS);
    }
}