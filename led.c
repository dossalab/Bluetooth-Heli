#include <nrf_gpio.h>
#include <nrf_gpiote.h>
#include <nrf_ppi.h>

#include "ppi-gpiote-map.h"
#include "timer.h"

#define LED_PIN			27
#define LED_INIT_VALUE		NRF_GPIOTE_INITIAL_VALUE_LOW
#define LED_VALUE_ON		NRF_GPIOTE_INITIAL_VALUE_HIGH
#define GPIOTE_LED_CHANNEL	0

void led_switch_on(void)
{
	nrf_gpiote_task_force(GPIOTE_LED_CHANNEL, LED_VALUE_ON);
}

void led_switch_blinking(bool on)
{
	if (on) {
		nrf_ppi_channel_enable(PPI_LED_CHANNEL);
	} else {
		nrf_ppi_channel_disable(PPI_LED_CHANNEL);
		nrf_gpiote_task_force(GPIOTE_LED_CHANNEL, LED_INIT_VALUE);
	}
}

void led_init(void)
{
	nrf_gpio_pin_dir_set(LED_PIN, NRF_GPIO_PIN_DIR_OUTPUT);

	nrf_gpiote_task_configure(GPIOTE_LED_CHANNEL, LED_PIN, NRF_GPIOTE_POLARITY_TOGGLE, LED_INIT_VALUE);
	nrf_gpiote_task_enable(GPIOTE_LED_CHANNEL);

	nrf_ppi_channel_endpoint_setup(PPI_LED_CHANNEL,
			(uint32_t)event_timer_overflow_event_address_get(),
			(uint32_t)nrf_gpiote_task_addr_get(nrf_gpiote_out_task_get(GPIOTE_LED_CHANNEL)));
}
