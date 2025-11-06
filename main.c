#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "nrf_gpio.h"

#define LED1 13
#define LED2 14
#define LED3 15
#define LED4 16
#define LED5 17

#define BUTTON1 11

const uint8_t led_sequence[] = {LED1, LED1, LED3, LED3, LED5};
const uint8_t sequence_length = sizeof(led_sequence) / sizeof(led_sequence[0]);

void gpio_init(void) {
    nrf_gpio_cfg_output(LED1);
    nrf_gpio_cfg_output(LED2);
    nrf_gpio_cfg_output(LED3);
    nrf_gpio_cfg_output(LED4);
    nrf_gpio_cfg_output(LED5);
    
    nrf_gpio_cfg_input(BUTTON1, NRF_GPIO_PIN_PULLUP);
}

void turn_off_all_leds(void) {
    nrf_gpio_pin_write(LED1, 1);
    nrf_gpio_pin_write(LED2, 1);
    nrf_gpio_pin_write(LED3, 1);
    nrf_gpio_pin_write(LED4, 1);
    nrf_gpio_pin_write(LED5, 1);
}

int main(void) {
    gpio_init();
    turn_off_all_leds();
    
    uint8_t current_led_index = 0;
    bool button_was_pressed = false;
    
    while (true) {
        bool button_is_pressed = !nrf_gpio_pin_read(BUTTON1);
        
        if (button_is_pressed) {
            nrf_gpio_pin_write(led_sequence[current_led_index], 0);
            nrf_delay_ms(1000);
            
            if (nrf_gpio_pin_read(BUTTON1) == 0) {
                nrf_gpio_pin_write(led_sequence[current_led_index], 1);
                current_led_index = (current_led_index + 1) % sequence_length;
            }
            button_was_pressed = true;
        } else {
            if (button_was_pressed) {
                button_was_pressed = false;
            } else {
                turn_off_all_leds();
            }
        }
    }
    
    return 0;
}