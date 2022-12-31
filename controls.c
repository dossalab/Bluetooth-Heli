#include <app_error.h>
#include "ble.h"

#define CONTROLS_SERVICE_UUID		0x53D3
#define CONTROLS_CHAR_UUID		0x53D4

typedef struct {
	uint8_t throttle;
	uint8_t rudder;
	uint8_t elevator;

	uint8_t reserved;
} control_packet;

static const ble_uuid128_t controls_full_uuid = {
	.uuid128 = { 0xEE, 0x10, 0x0c, 0x37, 0x14, 0x94, 0x4a, 0x17, \
		0xB0, 0x1D, 0x7F, 0x20, 0x00, 0x00, 0xD4, 0x05 }
};

static control_packet controls_char_init_value;
static ble_gatts_char_handles_t controls_char_handle;

static void handle_controls_write(const ble_gatts_evt_write_t *event)
{
	if (event->handle == controls_char_handle.value_handle && event->len == sizeof(control_packet)) {
		control_packet *packet = (void *)event->data;
	}
}

static void handle_events(const ble_evt_t *event, void *user)
{
	switch (event->header.evt_id) {
	case BLE_GATTS_EVT_WRITE:
		handle_controls_write(&event->evt.gatts_evt.params.write);
		break;
	}
}

NRF_SDH_BLE_OBSERVER(controls_service_observer, BLE_C_OBSERVERS_PRIORITY, handle_events, NULL);

static void controls_service_init(void)
{
	ret_code_t err;
	uint16_t service_handle;

	ble_uuid_t ble_service_uuid = {
		.uuid = CONTROLS_SERVICE_UUID,
		/* .type will be filled in by sd_ble_uuid_vs_add */
	};

	ble_add_char_params_t controls_char_params = {
		.uuid             = CONTROLS_CHAR_UUID,
		.max_len          = sizeof(control_packet),
		.init_len         = sizeof(control_packet),
		.p_init_value     = (void *)&controls_char_init_value,
		.char_props.read  = 1,
		.char_props.write = 1,
		.read_access      = SEC_OPEN,
		.write_access     = SEC_OPEN,
	};

	err = sd_ble_uuid_vs_add(&controls_full_uuid, &ble_service_uuid.type);
	APP_ERROR_CHECK(err);

	err = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, \
			&ble_service_uuid, &service_handle);

	err = characteristic_add(service_handle, &controls_char_params, &controls_char_handle);
	APP_ERROR_CHECK(err);
}

void controls_init(void)
{
	controls_service_init();
}
