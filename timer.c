#include <nrf_timer.h>
#include "ble.h"

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
#define EVENT_TIMER_PERIOD_FAST_MS	500U
#define EVENT_TIMER_PERIOD_SLOW_MS	2000U

static void timer_mode_fast(void)
{
	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_cc_write(EVENT_TIMER, 0, \
		nrf_timer_ms_to_ticks(EVENT_TIMER_PERIOD_FAST_MS, EVENT_TIMER_FREQUENCY));
}

static void timer_mode_slow(void)
{
	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_CLEAR);
	nrf_timer_cc_write(EVENT_TIMER, 0, \
		nrf_timer_ms_to_ticks(EVENT_TIMER_PERIOD_SLOW_MS, EVENT_TIMER_FREQUENCY));
}

static void connection_events_handler(ble_evt_t const *event, void *user)
{
	switch (event->header.evt_id) {
	case BLE_GAP_EVT_CONNECTED:
		timer_mode_fast();
		break;

	case BLE_GAP_EVT_DISCONNECTED:
		timer_mode_slow();
		break;
	}
}

NRF_SDH_BLE_OBSERVER(connection_observer, BLE_C_OBSERVERS_PRIORITY, connection_events_handler, NULL);

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

	timer_mode_slow();
	nrf_timer_task_trigger(EVENT_TIMER, NRF_TIMER_TASK_START);
}

