#pragma once

#include <nrf_gpio.h>
#include "resource-map.h"

static inline void led_off(void) {
	nrf_gpio_pin_clear(LED_PIN);
}

static inline void led_on(void) {
	nrf_gpio_pin_set(LED_PIN);
}

static inline void led_toggle(void) {
	nrf_gpio_pin_toggle(LED_PIN);
}

static inline void led_init(void) {
	nrf_gpio_pin_dir_set(LED_PIN, NRF_GPIO_PIN_DIR_OUTPUT);
}
