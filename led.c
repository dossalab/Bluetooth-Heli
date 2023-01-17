#include <nrf_gpio.h>
#include <nrf_gpiote.h>
#include <nrf_ppi.h>

#include "ble.h"
#include "ppi-gpiote-map.h"
#include "timer.h"
#include "extint.h"

#define LED_PIN			27
#define LED_INIT_VALUE		NRF_GPIOTE_INITIAL_VALUE_LOW

static bool ble_connected, charger_connected;

static void led_state_update(void)
{
	__COMPILER_BARRIER();
	if (ble_connected || charger_connected) {
		nrf_ppi_channel_enable(PPI_LED_CHANNEL);
	} else {
		nrf_ppi_channel_disable(PPI_LED_CHANNEL);
		nrf_gpiote_task_force(GPIOTE_LED_CHANNEL, LED_INIT_VALUE);
	}
}

static void connection_events_handler(ble_evt_t const *event, void *user)
{
	switch (event->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
		ble_connected = true;
		break;

	case BLE_GAP_EVT_DISCONNECTED:
		ble_connected = false;
		break;
	}

	led_state_update();
}

static void charger_event_handler(bool is_charging)
{
	charger_connected = is_charging;
	led_state_update();
}

NRF_SDH_BLE_OBSERVER(connection_observer, BLE_C_OBSERVERS_PRIORITY, connection_events_handler, NULL);
EXTINT_CHARGER_EVENT_HANDLER(charger_event_handler);

void led_init(void)
{
	nrf_gpio_pin_dir_set(LED_PIN, NRF_GPIO_PIN_DIR_OUTPUT);

	nrf_gpiote_task_configure(GPIOTE_LED_CHANNEL, LED_PIN, NRF_GPIOTE_POLARITY_TOGGLE, LED_INIT_VALUE);
	nrf_gpiote_task_enable(GPIOTE_LED_CHANNEL);

	nrf_ppi_channel_endpoint_setup(PPI_LED_CHANNEL,
			(uint32_t)event_timer_overflow_event_address_get(),
			(uint32_t)nrf_gpiote_task_addr_get(nrf_gpiote_out_task_get(GPIOTE_LED_CHANNEL)));

	led_state_update();
}
