/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup blinky_example_main main.c
 * @{
 * @ingroup blinky_example
 * @brief Blinky Example Application main file.
 *
 * This file contains the source code for a sample application to blink LEDs.
 *
 */

#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"

// WAIT TIME
#define WAIT_1_MSEC 1000
#define WAIT_2_MSEC 100

// LEDS
#define MY_LED_1 NRF_GPIO_PIN_MAP(0, 6)
#define LED_2_R NRF_GPIO_PIN_MAP(0, 8)
#define LED_2_G NRF_GPIO_PIN_MAP(1, 9)
#define LED_2_B NRF_GPIO_PIN_MAP(0, 12)

// SWITCH
#define SW_BUTTON NRF_GPIO_PIN_MAP(1, 6)

// nRF ID and LEDS
const int ID[] = {7, 3, 2, 4};
const uint32_t LEDS[] = {MY_LED_1, LED_2_R, LED_2_G, LED_2_B};

void init_switch(){
    nrf_gpio_cfg_input(SW_BUTTON, NRF_GPIO_PIN_PULLUP);
}

void init_led(const uint32_t led){
    nrf_gpio_cfg_output(led);
    nrf_gpio_pin_write(led, 1);
}

void ton_led(int i){
    nrf_gpio_pin_write(LEDS[i], 0);
}

void toff_led(int i){
    nrf_gpio_pin_write(LEDS[i], 1);
}

bool led_state(int i){
    return nrf_gpio_pin_read(LEDS[i]);
}

bool press_switch(){
    return !nrf_gpio_pin_read(SW_BUTTON);
}


int main(void)
{
    for (int i = 0; i < LEDS_NUMBER; i++){
        init_led(LEDS[i]);
    }
    init_switch();

    int led = 0;
    int counter = 0;
    int hold = 0;
    while(1){
        if (led == 3 && counter == ID[led]){
            led = 0;
            counter = 0;
        }
        if (counter == ID[led]){
            led += 1;
            counter = 0;
        }
        while(press_switch()){
            if (!led_state(led)){
                ton_led(led);
            }
            if (counter == ID[led]){
                break;
            }
            if (hold == WAIT_1_MSEC){
                counter++;
                hold = 0;
            }
            hold += WAIT_2_MSEC;
            nrf_delay_ms(WAIT_2_MSEC);
        }
        if (!led_state(led)){
            toff_led(led);
        }
        nrf_delay_ms(WAIT_1_MSEC);
    }
    return 0;
}
