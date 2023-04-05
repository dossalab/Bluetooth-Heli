#include <ble_bas/ble_bas.h>
#include <app_error.h>
#include <nrf_twim.h>
#include <nrf_gpio.h>
#include <nrf_atomic.h>

#include "resource-map.h"
#include "irq-prio.h"

static NRF_TWIM_Type * const twi = BATTERY_GAUGE_TWI;

#define BATTERY_GAUGE_TWI_FREQUENCY	NRF_TWIM_FREQ_100K
#define BATTERY_GAUGE_TWI_ADDRESS	0x55

typedef enum {
	BQ27427_CMD_TEMPERATURE = 0x02,
	BQ27427_CMD_VOLTAGE = 0x04,
	BQ27427_CMD_FLAGS = 0x06,
	BQ27427_CMD_SOC = 0x1C,
} battery_gauge_command_t;

static nrf_atomic_u32_t twi_error_counter;

/* We always send 1-byte commands, and expect 2-byte responses */
static uint8_t twi_command_buffer;
static uint16_t twi_response_buffer;

BLE_BAS_DEF(battery_service);

static bool battery_gauge_twi_busy(void)
{
	return nrf_twim_event_check(twi, NRF_TWIM_EVENT_RXSTARTED) ||
		nrf_twim_event_check(twi, NRF_TWIM_EVENT_TXSTARTED);
}

static bool battery_gauge_send_command(battery_gauge_command_t command)
{
	if (battery_gauge_twi_busy()) {
		return false;
	}

	twi_command_buffer = command;
	nrf_twim_task_trigger(twi, NRF_TWIM_TASK_STARTTX);

	return true;
}

static void battery_gauge_parse_response(void)
{
	switch (twi_command_buffer) {
	case BQ27427_CMD_SOC:
		ble_bas_battery_level_update(&battery_service,
				twi_response_buffer, BLE_CONN_HANDLE_ALL);
		break;

	default:
		break;
	}
}

void BATTERY_GAUGE_TWI_IRQ_HANDLER(void)
{
	/*
	 * We don't have those interrupts unmasked, just make sure they are cleared
	 * if stop or error is generated (so we know TWI is not busy anymore)
	 */
	nrf_twim_event_clear(twi, NRF_TWIM_EVENT_RXSTARTED);
	nrf_twim_event_clear(twi, NRF_TWIM_EVENT_TXSTARTED);

	if (nrf_twim_event_check(twi, NRF_TWIM_EVENT_ERROR)) {
		nrf_twim_event_clear(twi, NRF_TWIM_EVENT_ERROR);
		nrf_twim_errorsrc_get_and_clear(twi);

		nrf_atomic_u32_add(&twi_error_counter, 1);
	}

	if (nrf_twim_event_check(twi, NRF_TWIM_EVENT_STOPPED)) {
		nrf_twim_event_clear(twi, NRF_TWIM_EVENT_STOPPED);

		if (nrf_twim_rxd_amount_get(twi) == sizeof(twi_response_buffer)) {
			battery_gauge_parse_response();
		}
	}
}

static void battery_service_init(void)
{
	ret_code_t err_code;

	ble_bas_init_t batt_service_config = {
		.evt_handler = NULL,
		.support_notification = true,
		.p_report_ref = NULL,
		.initial_batt_level = 0,
		.bl_rd_sec = SEC_OPEN,
		.bl_cccd_wr_sec = SEC_OPEN,
		.bl_report_rd_sec = SEC_OPEN,
	};

	err_code = ble_bas_init(&battery_service, &batt_service_config);
	APP_ERROR_CHECK(err_code);
}

static void battery_gauge_twi_init(void)
{
	nrf_twim_pins_set(twi, BATTERY_GAUGE_PIN_TWI_SCL, BATTERY_GAUGE_PIN_TWI_SDA);
	nrf_twim_frequency_set(twi, BATTERY_GAUGE_TWI_FREQUENCY);
	nrf_twim_address_set(twi, BATTERY_GAUGE_TWI_ADDRESS);

	nrf_twim_tx_buffer_set(twi, &twi_command_buffer, sizeof(twi_command_buffer));
	nrf_twim_rx_buffer_set(twi, (void *)&twi_response_buffer, sizeof(twi_response_buffer));

	nrf_twim_int_enable(twi, NRF_TWIM_INT_STOPPED_MASK | NRF_TWIM_INT_ERROR_MASK);
	nrf_twim_shorts_enable(twi, NRF_TWIM_SHORT_LASTTX_STARTRX_MASK
			| NRF_TWIM_SHORT_LASTRX_STOP_MASK);

	nrf_twim_enable(BATTERY_GAUGE_TWI);

	NVIC_SetPriority(BATTERY_GAUGE_TWI_NVIC_IRQN, TWI_IRQ_PRIORITY);
	NVIC_EnableIRQ(BATTERY_GAUGE_TWI_NVIC_IRQN);
}

static void battery_gauge_init(void)
{
	nrf_gpio_cfg_sense_input(BATTERY_GAUGE_PIN_INT,
			NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

	battery_gauge_twi_init();
	battery_gauge_send_command(BQ27427_CMD_SOC);
}

void battery_monitor_init(void)
{
	battery_service_init();
	battery_gauge_init();
}
