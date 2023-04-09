#include <nrf_soc.h>

#include "ble.h"
#include "controls.h"
#include "motors.h"
#include "power.h"
#include "led.h"
#include "version.h"
#include "gyro.h"

#define MAX_CONNECTION_INTERVAL		MSEC_TO_UNITS(20, UNIT_1_25_MS)
#define SUPERVISION_INTERVAL		MSEC_TO_UNITS(500, UNIT_1_25_MS)

static void ble_init(void)
{
	ble_c_init_with_name(DEVICE_MODEL_STRING);
	ble_c_set_max_connection_interval(MAX_CONNECTION_INTERVAL);
	ble_c_set_supervision_timeout(SUPERVISION_INTERVAL);
}

int main(void)
{
	led_init();
	motors_init();

	ble_init();
	controls_init();
	gyro_init();
	power_module_init();
	version_service_init();

	for (;;) {
		sd_app_evt_wait();
	}
}
