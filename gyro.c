#include <nrf_saadc.h>
#include <nrf_ppi.h>
#include <nrf_gpio.h>

#include "ble.h"
#include "resource-map.h"
#include "irq-prio.h"

#define	_adc_bits_to_resolution(x)	NRF_SAADC_RESOLUTION_##x##BIT
#define	ADC_BITS_TO_RESOLUTION(bits)	_adc_bits_to_resolution(bits)

/* This is a fixed voltage regulator that we use as a reference */
#define ADC_INTERNAL_VREF_MV		600

#define ADC_RESOLUTION_BITS		10
#define ADC_RANGE			(1U << ((ADC_RESOLUTION_BITS) - 1))

#define GYRO_ADC_CHANNEL		1

static nrf_saadc_value_t gyro_measurement_result;

static inline int32_t measurement_to_angular_speed(nrf_saadc_value_t value) {
	int32_t mv = value * ADC_RESOLUTION_BITS / ADC_INTERNAL_VREF_MV;
	return mv * 3 / 2;
}

static inline void gyro_power_enable(void) {
	nrf_gpio_pin_set(GYRO_POWER_ENABLE_PIN);
}

static inline void gyro_power_disable(void) {
	nrf_gpio_pin_clear(GYRO_POWER_ENABLE_PIN);
}

static void run_pid_loop(int32_t angular)
{
	/* TODO - run pid loop */
}

void SAADC_IRQHandler(void)
{
	int32_t angular;

	if (nrf_saadc_event_check(NRF_SAADC_EVENT_CALIBRATEDONE)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_CALIBRATEDONE);
	}

	if (nrf_saadc_event_check(NRF_SAADC_EVENT_STARTED)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_STARTED);
		nrf_saadc_task_trigger(NRF_SAADC_TASK_SAMPLE);
	}

	if (nrf_saadc_event_check(NRF_SAADC_EVENT_END)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_END);

		angular = measurement_to_angular_speed(gyro_measurement_result);
		run_pid_loop(angular);

		nrf_saadc_task_trigger(NRF_SAADC_TASK_STOP);
	}

	if (nrf_saadc_event_check(NRF_SAADC_EVENT_STOPPED)) {
		nrf_saadc_event_clear(NRF_SAADC_EVENT_STOPPED);
	}
}

static void adc_start_calibrate(void)
{
	nrf_saadc_task_trigger(NRF_SAADC_TASK_CALIBRATEOFFSET);
}

static void gyro_adc_init(void)
{
	nrf_saadc_channel_config_t config = {
		.resistor_p = NRF_SAADC_RESISTOR_DISABLED,
		.resistor_n = NRF_SAADC_RESISTOR_DISABLED,
		.acq_time = NRF_SAADC_ACQTIME_40US,
		.mode = NRF_SAADC_MODE_DIFFERENTIAL,
		.pin_p = GYRO_ADC_MEASUREMENT_INPUT,
		.pin_n = GYRO_ADC_REFERENCE_INPUT,
		.burst = NRF_SAADC_BURST_ENABLED,
		.gain = NRF_SAADC_GAIN1,
		.reference = NRF_SAADC_REFERENCE_INTERNAL,
	};

	nrf_gpio_pin_dir_set(GYRO_POWER_ENABLE_PIN, NRF_GPIO_PIN_DIR_OUTPUT);

	nrf_saadc_enable();
	nrf_saadc_oversample_set(NRF_SAADC_OVERSAMPLE_16X);
	nrf_saadc_resolution_set(ADC_BITS_TO_RESOLUTION(ADC_RESOLUTION_BITS));
	nrf_saadc_buffer_init(&gyro_measurement_result, 1);
	nrf_saadc_channel_init(GYRO_ADC_CHANNEL, &config);

	nrf_saadc_int_enable(NRF_SAADC_INT_STARTED | NRF_SAADC_INT_END
		| NRF_SAADC_INT_STOPPED | NRF_SAADC_INT_CALIBRATEDONE);

	NVIC_SetPriority(SAADC_IRQn, USER_IRQ_PRIORITY);
	NVIC_EnableIRQ(SAADC_IRQn);

	adc_start_calibrate();
}

static void ble_events_handler(ble_evt_t const *event, void *user)
{
	switch (event->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
		gyro_power_enable();
		break;

	case BLE_GAP_EVT_DISCONNECTED:
		gyro_power_disable();
		break;
	}
}

NRF_SDH_BLE_OBSERVER(gyro_connection_observer, BLE_C_OBSERVERS_PRIORITY, ble_events_handler, NULL);

void gyro_init(void)
{
	gyro_adc_init();
}
