#include <nrf_soc.h>
#include "ble.h"
#include "controls.h"
#include "motors.h"
#include "power.h"
#include "timer.h"
#include "led.h"

#define MAX_CONNECTION_INTERVAL		MSEC_TO_UNITS(20, UNIT_1_25_MS)

static void connection_events_handler(ble_evt_t const *event, void *user)
{
	switch (event->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
		led_switch_blinking(true);
		break;

	case BLE_GAP_EVT_DISCONNECTED:
		led_switch_blinking(false);
		break;
	}
}

NRF_SDH_BLE_OBSERVER(connection_observer, BLE_C_OBSERVERS_PRIORITY, connection_events_handler, NULL);

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t _info)
{
	__disable_irq();
	motors_disarm();
	led_switch_on();

	while(1);
}

int main(void)
{
	event_timer_init();
	led_init();
	motors_init();
	ble_c_init();
	ble_c_set_max_connection_interval(MAX_CONNECTION_INTERVAL);
	controls_init();
	power_management_init();

	for (;;) {
		sd_app_evt_wait();
	}
}