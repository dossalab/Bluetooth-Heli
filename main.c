#include <nrf_soc.h>

#include "ble.h"
#include "controls.h"
#include "motors.h"
#include "battery.h"
#include "timer.h"
#include "led.h"
#include "extint.h"

#define MAX_CONNECTION_INTERVAL		MSEC_TO_UNITS(20, UNIT_1_25_MS)
#define SUPERVISION_INTERVAL		MSEC_TO_UNITS(500, UNIT_1_25_MS)

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t _info)
{
	__disable_irq();
	motors_disarm();

	while(1);
}

int main(void)
{
	event_timer_init();
	led_init();
	motors_init();
	ble_c_init();
	ble_c_set_max_connection_interval(MAX_CONNECTION_INTERVAL);
	ble_c_set_supervision_timeout(SUPERVISION_INTERVAL);
	controls_init();
	battery_monitor_init();
	external_interrupts_init();

	for (;;) {
		sd_app_evt_wait();
	}
}