#include <ble_bas/ble_bas.h>
#include <app_error.h>

#define BATTERY_INITIAL_LEVEL		0

BLE_BAS_DEF(battery_service);

// TODO - talk to the battery gauge

static void battery_service_init(void)
{
	ret_code_t err_code;

	ble_bas_init_t batt_service_config = {
		.evt_handler = NULL,
		.support_notification = true,
		.p_report_ref = NULL,
		.initial_batt_level = BATTERY_INITIAL_LEVEL,
		.bl_rd_sec = SEC_OPEN,
		.bl_cccd_wr_sec = SEC_OPEN,
		.bl_report_rd_sec = SEC_OPEN,
	};

	err_code = ble_bas_init(&battery_service, &batt_service_config);
	APP_ERROR_CHECK(err_code);
}

void battery_monitor_init(void)
{
	battery_service_init();
}
