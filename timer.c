#include <nrf_ppi.h>
#include <nrf_rtc.h>

#include "ppi-gpiote-map.h"
#include "ble.h"
#include "extint.h"

/*
 * This module is intended to be used together with PPI to generate events
 * for different peripherals - for example for battery measurements.
 *
 * We could've used multiple timers or even the app_timer instead, but this
 * approach is more battery-saving, since there is only one hardware RTC running
 * and there is almost no CPU involvment because of the PPI.
 *
 * RTC0 and higher PPI channels are reserved by the SoftDevice -
 * see nrf_soc.h header and SoftDevice specification for more details.
 * Some channels in that range are even preprogrammed and can't be changed at all.
 */

#define EVENT_RTC			NRF_RTC1

/*
 * We can't set how fast overflows are occuring, since the TOP value is fixed. However,
 * we can use compare event and emulate the overflow by connecting it to the clear task
 */
#define EVENT_RTC_COMPARE_CLEAR_CHANNEL	0

/*
 * When we are connected to a charger or when the user opens a connection we can
 * start polling peripherals more often - in such cases we don't care about the battery
 *
 * The lowest frequency that prescaler will allow us to go is 8 Hz
 */

#define EVENT_RTC_FREQ_HZ		8

#define RTC_MS_TO_TICKS(ms)		((ms) * EVENT_RTC_FREQ_HZ / 1000U)

#define EVENT_RTC_PERIOD_FASTER_TICKS	RTC_MS_TO_TICKS(125U)
#define EVENT_RTC_PERIOD_FAST_TICKS	RTC_MS_TO_TICKS(500U)
#define EVENT_RTC_PERIOD_SLOW_TICKS	RTC_MS_TO_TICKS(60000U)

static bool charger_connected, ble_connected;

static void rtc_mode_update(void)
{
	uint32_t ticks;

	__COMPILER_BARRIER();

	if (charger_connected && ble_connected) {
		ticks = EVENT_RTC_PERIOD_FASTER_TICKS;
	} else if (charger_connected || ble_connected) {
		ticks = EVENT_RTC_PERIOD_FAST_TICKS;
	} else {
		ticks = EVENT_RTC_PERIOD_SLOW_TICKS;
	}

	nrf_rtc_cc_set(EVENT_RTC, EVENT_RTC_COMPARE_CLEAR_CHANNEL, ticks);
	nrf_rtc_task_trigger(EVENT_RTC, NRF_RTC_TASK_CLEAR);
}

static void ble_events_handler(ble_evt_t const *event, void *user)
{
	switch (event->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
		ble_connected = true;
		rtc_mode_update();
		break;

	case BLE_GAP_EVT_DISCONNECTED:
		ble_connected = false;
		rtc_mode_update();
		break;
	}
}

static void charger_event_handler(bool is_charging)
{
	charger_connected = is_charging;
	rtc_mode_update();
}

NRF_SDH_BLE_OBSERVER(connection_observer, BLE_C_OBSERVERS_PRIORITY, ble_events_handler, NULL);
EXTINT_CHARGER_EVENT_HANDLER(charger_event_handler);

uint32_t event_timer_overflow_event_address_get(void)
{
	return nrf_rtc_event_address_get(EVENT_RTC,
		nrf_rtc_compare_event_get(EVENT_RTC_COMPARE_CLEAR_CHANNEL));
}

void event_timer_init(void)
{
	nrf_rtc_prescaler_set(EVENT_RTC, RTC_FREQ_TO_PRESCALER(EVENT_RTC_FREQ_HZ));
	nrf_rtc_event_enable(EVENT_RTC, RTC_CHANNEL_INT_MASK(EVENT_RTC_COMPARE_CLEAR_CHANNEL));

	/* RTC lacks compare -> clear shortcut, so emulate it using PPI */
	nrf_ppi_channel_endpoint_setup(PPI_RTC_COMPARE_CLEAR_CHANNEL,
		nrf_rtc_event_address_get(EVENT_RTC, nrf_rtc_compare_event_get(EVENT_RTC_COMPARE_CLEAR_CHANNEL)),
		nrf_rtc_task_address_get(EVENT_RTC, NRF_RTC_TASK_CLEAR));
	nrf_ppi_channel_enable(PPI_RTC_COMPARE_CLEAR_CHANNEL);

	rtc_mode_update();
	nrf_rtc_task_trigger(EVENT_RTC, NRF_RTC_TASK_START);
}
