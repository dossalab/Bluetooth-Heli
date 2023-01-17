#include <nrf_timer.h>

#include "ble.h"
#include "extint.h"

/*
 * This module is intended to be used together with PPI to generate events
 * for different peripherals - for example for battery measurements.
 *
 * We could've used multiple timers or even the app_timer instead, but this
 * approach is more battery-saving, since there is only one hardware timer running
 * and there is almost no CPU involvment because of the PPI.
 *
 * Timer0 and higher PPI channels are reserved by the SoftDevice -
 * see nrf_soc.h header and SoftDevice specification for more details.
 * Some channels in that range are even preprogrammed and can't be changed at all.
 */

#define EVENT_TIMER			NRF_TIMER1
#define EVENT_TIMER_FREQUENCY		NRF_TIMER_FREQ_125kHz

/* If we're not connected there is no point of polling peripherals that often */
#define EVENT_TIMER_PERIOD_FAST_MS	300U
#define EVENT_TIMER_PERIOD_SLOW_MS	10000U

static bool charger_connected, ble_connected;

static void timer_mode_update(void)
{
	uint32_t ticks;

	__COMPILER_BARRIER();
	if (charger_connected || ble_connected) {
		ticks = nrf_timer_ms_to_ticks(EVENT_TIMER_PERIOD_FAST_MS, EVENT_TIMER_FREQUENCY);
	} else {
		ticks = nrf_timer_ms_to_ticks(EVENT_TIMER_PERIOD_SLOW_MS, EVENT_TIMER_FREQUENCY);
	}

	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_cc_write(EVENT_TIMER, NRF_TIMER_CC_CHANNEL0, ticks);
}

static void connection_events_handler(ble_evt_t const *event, void *user)
{
	switch (event->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
		ble_connected = true;
		break;

	case BLE_GAP_EVT_DISCONNECTED:
		ble_connected = false;
		break;
	}

	timer_mode_update();
}

static void charger_event_handler(bool is_charging)
{
	charger_connected = is_charging;
	timer_mode_update();
}

NRF_SDH_BLE_OBSERVER(connection_observer, BLE_C_OBSERVERS_PRIORITY, connection_events_handler, NULL);
EXTINT_CHARGER_EVENT_HANDLER(charger_event_handler);

uint32_t *event_timer_overflow_event_address_get(void)
{
	return nrf_timer_event_address_get(EVENT_TIMER, NRF_TIMER_EVENT_COMPARE0);
}

void event_timer_init(void)
{
	nrf_timer_frequency_set(EVENT_TIMER, EVENT_TIMER_FREQUENCY);
	nrf_timer_mode_set(EVENT_TIMER, NRF_TIMER_MODE_TIMER);
	nrf_timer_bit_width_set(EVENT_TIMER, NRF_TIMER_BIT_WIDTH_32);

	/* Make timer count from 0 to compare channel 0, clear on compare */
	nrf_timer_shorts_enable(EVENT_TIMER, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK);

	timer_mode_update();
	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_START);
}

