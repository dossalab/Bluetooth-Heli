#include <ble_bas/ble_bas.h>
#include <nrf_saadc.h>
#include <nrf_ppi.h>
#include <nrf_gpio.h>

#include "timer.h"
#include "ppi-gpiote-map.h"

/*
 * This must be lower than SoftDevice IRQ priorities, otherwise we'll get stuck
 * while caling Bluetooth-related APIs from the IRQ context
 */
#define ADC_IRQ_PRIORITY		6

#define BATTERY_ADC_CHANNEL		1
#define BATTERY_ADC_INPUT		NRF_SAADC_INPUT_AIN0
#define BATTERY_INITIAL_LEVEL		0

#define	_adc_bits_to_resolution(x)	NRF_SAADC_RESOLUTION_##x##BIT
#define	ADC_BITS_TO_RESOLUTION(bits)	_adc_bits_to_resolution(bits)

#define _adc_prescaler_to_gain(x)	NRF_SAADC_GAIN1_##x
#define ADC_PRESCALER_TO_GAIN(presc)	_adc_prescaler_to_gain(presc)

/*
 * Battery divider is controlled by a mosfet connected to this pin
 * We have to switch this mosfet on in order to get the measurement
 */
#define BATTERY_DIVIDER_ENABLE_PIN	1

static nrf_saadc_value_t battery_measurement_result;
BLE_BAS_DEF(battery_service);

struct battery_conversion_table_entry {
	unsigned adc;
	unsigned charge;
};

/*
 * The battery is connected through resistive divider formed by R1 and R2
 * All voltage levels are in mV for simplicity
 */

/* This is a fixed voltage regulator that we are using as a reference */
#define ADC_INTERNAL_VREF		600

#define ADC_RESOLUTION_BITS		10
#define ADC_MAXVAL			((1 << (ADC_RESOLUTION_BITS)) - 1)
#define BATTERY_DIVIDER_R1		220
#define BATTERY_DIVIDER_R2		220
#define BATTERY_ADC_PRESCALER		4

#define BATTERY_ADC_VALUE(voltage) \
	(voltage) * BATTERY_DIVIDER_R1 / (BATTERY_DIVIDER_R1 + BATTERY_DIVIDER_R2) * \
		 ADC_MAXVAL / ADC_INTERNAL_VREF / BATTERY_ADC_PRESCALER

/* https://lygte-info.dk/info/BatteryChargePercent%20UK.html */
static struct battery_conversion_table_entry battery_conversion_table[] = {
	{ .adc = BATTERY_ADC_VALUE(4200U), .charge = 100 },
	{ .adc = BATTERY_ADC_VALUE(4100U), .charge = 91 },
	{ .adc = BATTERY_ADC_VALUE(4000U), .charge = 79 },
	{ .adc = BATTERY_ADC_VALUE(3900U), .charge = 62 },
	{ .adc = BATTERY_ADC_VALUE(3800U), .charge = 42 },
	{ .adc = BATTERY_ADC_VALUE(3700U), .charge = 12 },
	{ .adc = BATTERY_ADC_VALUE(3600U), .charge = 2 },
	{ .adc = BATTERY_ADC_VALUE(3500U), .charge = 0 },
};

#define BATTERY_CONVERSION_TABLE_LEN \
		(sizeof(battery_conversion_table) / sizeof(battery_conversion_table[0]))

static inline unsigned approximate(int adc, int c1, int c2, int adc1, int adc2)
{
	return c1 + (c2 - c1) * (adc - adc1) / (adc2 - adc1);
}

static unsigned battery_value_lookup(unsigned adc)
{
	struct battery_conversion_table_entry *high_entry, *low_entry;

	for (int prev = 0, next = 1; next < BATTERY_CONVERSION_TABLE_LEN; prev++, next++) {
		high_entry = &battery_conversion_table[prev];
		low_entry = &battery_conversion_table[next];

		if (adc > low_entry->adc) {
			return approximate(adc, low_entry->charge, high_entry->charge,
					low_entry->adc, high_entry->adc);
		}
	}

	return 0;
}

static inline void battery_divider_enable(void) {
	nrf_gpio_pin_set(BATTERY_DIVIDER_ENABLE_PIN);
}

static inline void battery_divider_disable(void) {
	nrf_gpio_pin_clear(BATTERY_DIVIDER_ENABLE_PIN);
}

void SAADC_IRQHandler(void)
{
	unsigned charge;

	if (nrf_saadc_event_check(NRF_SAADC_EVENT_CALIBRATEDONE)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_CALIBRATEDONE);
	}

	if (nrf_saadc_event_check(NRF_SAADC_EVENT_STARTED)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_STARTED);

		/* We are ready for the measurement - trigger it */
		battery_divider_enable();
		nrf_saadc_task_trigger(NRF_SAADC_TASK_SAMPLE);
	}

	if (nrf_saadc_event_check(NRF_SAADC_EVENT_END)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_END);

		/* Measurement is done */
		battery_divider_disable();
		charge = battery_value_lookup(battery_measurement_result);

		/* TODO - is it ok to call it from the interrupt context? */
		ble_bas_battery_level_update(&battery_service, charge, BLE_CONN_HANDLE_ALL);
		nrf_saadc_task_trigger(NRF_SAADC_TASK_STOP);
	}

	if (nrf_saadc_event_check(NRF_SAADC_EVENT_STOPPED)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_STOPPED);
	}
}

static void battery_adc_start_calibrate(void)
{
	nrf_saadc_task_trigger(NRF_SAADC_TASK_CALIBRATEOFFSET);
}

static void battery_adc_init(void)
{
	nrf_saadc_channel_config_t config = {
		.resistor_p = NRF_SAADC_RESISTOR_DISABLED,
		.resistor_n = NRF_SAADC_RESISTOR_DISABLED,
		.acq_time = NRF_SAADC_ACQTIME_40US,
		.mode = NRF_SAADC_MODE_SINGLE_ENDED,
		.pin_p = BATTERY_ADC_INPUT,
		.pin_n = NRF_SAADC_INPUT_DISABLED,
		.burst = NRF_SAADC_BURST_ENABLED,
		.gain = ADC_PRESCALER_TO_GAIN(BATTERY_ADC_PRESCALER),
		.reference = NRF_SAADC_REFERENCE_INTERNAL,
	};

	nrf_gpio_pin_dir_set(BATTERY_DIVIDER_ENABLE_PIN, NRF_GPIO_PIN_DIR_OUTPUT);

	nrf_saadc_enable();
	nrf_saadc_oversample_set(NRF_SAADC_OVERSAMPLE_128X);
	nrf_saadc_resolution_set(ADC_BITS_TO_RESOLUTION(ADC_RESOLUTION_BITS));
	nrf_saadc_buffer_init(&battery_measurement_result, 1);
	nrf_saadc_channel_init(BATTERY_ADC_CHANNEL, &config);

	nrf_saadc_int_enable(NRF_SAADC_INT_STARTED | NRF_SAADC_INT_END
		| NRF_SAADC_INT_STOPPED | NRF_SAADC_INT_CALIBRATEDONE);
	NVIC_SetPriority(SAADC_IRQn, ADC_IRQ_PRIORITY);
	NVIC_EnableIRQ(SAADC_IRQn);

	battery_adc_start_calibrate();
}

static void battery_adc_ppi_hookup(void)
{
	nrf_ppi_channel_endpoint_setup(PPI_ADC_CHANNEL,
			event_timer_overflow_event_address_get(),
			(uint32_t)nrf_saadc_task_address_get(NRF_SAADC_TASK_START));
	nrf_ppi_channel_enable(PPI_ADC_CHANNEL);
}

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
	battery_adc_init();
	battery_adc_ppi_hookup();
}
