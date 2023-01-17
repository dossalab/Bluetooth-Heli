#include <nrf_gpio.h>

#include "ble.h"
#include "controls.h"
#include "motors.h"
#include "power.h"
#include "timer.h"

#define LED_PIN		27
#define MAX_CONNECTION_INTERVAL		MSEC_TO_UNITS(20, UNIT_1_25_MS)

static inline void led_switch(bool on) {
	nrf_gpio_pin_write(LED_PIN, on);
}

static inline void led_init(void) {
	nrf_gpio_pin_dir_set(LED_PIN, NRF_GPIO_PIN_DIR_OUTPUT);
	led_switch(false);
}

static void connection_events_handler(ble_evt_t const *event, void *user)
{
	switch (event->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
		led_switch(true);
		break;

	case BLE_GAP_EVT_DISCONNECTED:
		led_switch(false);
		break;
	}
}

NRF_SDH_BLE_OBSERVER(connection_observer, BLE_C_OBSERVERS_PRIORITY, connection_events_handler, NULL);

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t _info)
{
	__disable_irq();
	motors_disarm();
	led_switch(true);

	while(1);
}

int main(void)
{
	led_init();
	motors_init();
	ble_c_init();
	ble_c_set_max_connection_interval(MAX_CONNECTION_INTERVAL);
	controls_init();
	power_management_init();
	event_timer_init();

	for (;;) {
		sd_app_evt_wait();
	}
}