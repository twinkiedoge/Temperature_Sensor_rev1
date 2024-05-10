#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <stdio.h>

gpio_num_t load_gpio[] = {6, 7, 10};

void set_load_gpios(uint32_t value) {
    for(int i = 0; i < sizeof(load_gpio) / sizeof(gpio_num_t); i++) {
        gpio_set_level(load_gpio[i], value);
    }
}

void keep_battery_alive(void* params) {

    for(int i = 0; i < sizeof(load_gpio) / sizeof(gpio_num_t); i++) {
        gpio_reset_pin(load_gpio[i]);
        gpio_set_direction(load_gpio[i], GPIO_MODE_OUTPUT);
        // Disable the pullups; no point in wasting current while output is low
        gpio_set_pull_mode(load_gpio[i], GPIO_PULLUP_DISABLE);
    }

    while(1){
        printf("Drawing current to stay alive!\n");
        set_load_gpios(1);
        vTaskDelay(150); // 1.5 seconds on

        printf("Load off\n");
        set_load_gpios(0);
        vTaskDelay(850);  // 8.5 seconds off
    }
}