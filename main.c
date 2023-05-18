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

/*
 * In order to simplify working with 128-bit UUIDs the SDK allows us to
 * register such identifiers globally with `sd_ble_uuid_vs_add`.
 *
 * That function returns a handle that is used later for creating custom services.
 */
static unsigned register_base_uuid(void)
{
	ret_code_t err;
	uint8_t base_uuid_handle;

	const ble_uuid128_t base_uuid = {
		.uuid128 = {
			0xEE, 0x10, 0x0c, 0x37, 0x14, 0x94, 0x4a, 0x17, \
			0xB0, 0x1D, 0x7F, 0x20, 0x00, 0x00, 0xD4, 0x03
		}
	};

	err = sd_ble_uuid_vs_add(&base_uuid, &base_uuid_handle);
	APP_ERROR_CHECK(err);

	return base_uuid_handle;
}

static unsigned ble_init(void)
{
	ble_c_init_with_name(DEVICE_MODEL_STRING);
	ble_c_set_max_connection_interval(MAX_CONNECTION_INTERVAL);
	ble_c_set_supervision_timeout(SUPERVISION_INTERVAL);

	return register_base_uuid();
}

int main(void)
{
	unsigned base_uuid_handle;

	led_init();
	motors_init();

	base_uuid_handle = ble_init();

	controls_init(base_uuid_handle);
	gyro_init();
	power_module_init();
	version_service_init();

	for (;;) {
		sd_app_evt_wait();
	}
}
