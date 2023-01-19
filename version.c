#include <ble_dis/ble_dis.h>
#include <app_error.h>

#include "version.h"

void version_service_init(void)
{
	ret_code_t err;

	ble_dis_init_t init_params = {
		.dis_char_rd_sec = SEC_OPEN,
	};

	ble_srv_ascii_to_utf8(&init_params.manufact_name_str, DEVICE_MANUFACTURER_STRING);
	ble_srv_ascii_to_utf8(&init_params.model_num_str, DEVICE_MODEL_STRING);
	ble_srv_ascii_to_utf8(&init_params.fw_rev_str, SW_VERSION_STRING);

	err = ble_dis_init(&init_params);
	APP_ERROR_CHECK(err);
}
