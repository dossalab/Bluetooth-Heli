#include <ble_bas/ble_bas.h>
#include <app_error.h>
#include <nrf_twim.h>
#include <nrf_gpio.h>
#include <nrf_gpiote.h>
#include <nrf_atomic.h>
#include <nrf_rtc.h>

#include "resource-map.h"
#include "irq-prio.h"
#include "led.h"

static NRF_TWIM_Type * const twi = BATTERY_GAUGE_TWI;
static NRF_RTC_Type * const rtc = PMIC_MONITOR_RTC;

#define PMIC_MONITOR_FREQ_HZ		8
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

static inline bool pmic_is_charging(void) {
	return !nrf_gpio_pin_read(PMIC_CHARGE_DETECT_PIN);
}

static inline bool pmic_is_failure(void) {
	return !nrf_gpio_pin_read(PMIC_FAILURE_DETECT_PIN);
}

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

	nrf_twim_enable(twi);
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
		nrf_twim_disable(twi);
	}

	if (nrf_twim_event_check(twi, NRF_TWIM_EVENT_STOPPED)) {
		nrf_twim_event_clear(twi, NRF_TWIM_EVENT_STOPPED);

		if (nrf_twim_rxd_amount_get(twi) == sizeof(twi_response_buffer)) {
			battery_gauge_parse_response();
		}

		nrf_twim_disable(twi);
	}
}


void PMIC_MONITOR_RTC_IRQ_HANDLER(void)
{
	if (nrf_rtc_event_pending(rtc, NRF_RTC_EVENT_TICK)) {
		nrf_rtc_event_clear(rtc, NRF_RTC_EVENT_TICK);

		if (!pmic_is_charging() && !pmic_is_failure()) {
			led_off();
			nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_STOP);
		}

		if (pmic_is_failure()) {
			led_on();
		} else if (pmic_is_charging()) {
			led_toggle();
		}
	}
}

/*
 * TODO: in the future we might need to share this IRQ handler with others.
 */
void GPIOTE_IRQHandler(void)
{
	if (nrf_gpiote_event_is_set(NRF_GPIOTE_EVENTS_PORT)) {
		nrf_gpiote_event_clear(NRF_GPIOTE_EVENTS_PORT);

		if (pmic_is_charging() || pmic_is_failure()) {
			nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_START);
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

/*
 * After we detect charge or failure we want to let user know what is happening
 * So prepare RTC here and run it later to monitor status and blink LED
 */
static void pmic_monitor_init(void)
{
	nrf_gpio_cfg_sense_input(PMIC_CHARGE_DETECT_PIN,
			NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);
	nrf_gpio_cfg_sense_input(PMIC_FAILURE_DETECT_PIN,
			NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

	nrf_rtc_prescaler_set(rtc, RTC_FREQ_TO_PRESCALER(PMIC_MONITOR_FREQ_HZ));
	nrf_rtc_int_enable(rtc, NRF_RTC_INT_TICK_MASK);
	nrf_rtc_task_trigger(rtc, NRF_RTC_TASK_START);

	NVIC_SetPriority(PMIC_MONITOR_RTC_IRQN, USER_IRQ_PRIORITY);
	NVIC_EnableIRQ(PMIC_MONITOR_RTC_IRQN);
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

	NVIC_SetPriority(BATTERY_GAUGE_TWI_NVIC_IRQN, USER_IRQ_PRIORITY);
	NVIC_EnableIRQ(BATTERY_GAUGE_TWI_NVIC_IRQN);
}

static void battery_gauge_init(void)
{
	nrf_gpio_cfg_sense_input(BATTERY_GAUGE_PIN_INT,
			NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

	battery_gauge_twi_init();
	battery_gauge_send_command(BQ27427_CMD_SOC);
}

void power_module_init(void)
{
	battery_service_init();
	pmic_monitor_init();
	battery_gauge_init();

	nrf_gpiote_int_enable(NRF_GPIOTE_INT_PORT_MASK);
	NVIC_SetPriority(GPIOTE_IRQn, USER_IRQ_PRIORITY);
	NVIC_EnableIRQ(GPIOTE_IRQn);
}
