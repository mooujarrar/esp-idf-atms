#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    uint64_t output_pin_bit_mask = (1ULL << GPIO_NUM_2);
    gpio_config_t output_pin_config = { output_pin_bit_mask, GPIO_MODE_OUTPUT};
    if (gpio_config(&output_pin_config) == ESP_OK) {
        while(true) {
            gpio_set_level(GPIO_NUM_2, 1);
            vTaskDelay(500 / portTICK_PERIOD_MS);
            gpio_set_level(GPIO_NUM_2, 0);
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}
