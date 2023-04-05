#include <nrf_gpio.h>
#include <nrf_gpiote.h>

#include "extint.h"
#include "resource-map.h"

#define _gpiote_channel_to_int(x)		NRF_GPIOTE_INT_IN##x##_MASK
#define GPIOTE_CHANNEL_TO_INT(channel)		_gpiote_channel_to_int(channel)
#define _gpiote_channel_to_event(x)		NRF_GPIOTE_EVENTS_IN_##x
#define GPIOTE_CHANNEL_TO_EVENT(channel)	_gpiote_channel_to_event(channel)

NRF_SECTION_SET_DEF(EXTINT_CHARGER_EVENT_SECTION, extint_charger_event_handler, 1);

/*
 * Those pins are connected to nPM1100 and may be used to sense the
 * battery failure and charger status
 *
 * Both of those are open drain and require pullups (hence the value is inverted)
 */
#define PMIC_CHARGE_DETECT_PIN		11
#define PMIC_FAILURE_DETECT_PIN		12

static void notify_charger_event_observers(bool charging)
{
	nrf_section_iter_t iter;
	extint_charger_event_handler *handler;

	for (nrf_section_iter_init(&iter, &EXTINT_CHARGER_EVENT_SECTION);
		nrf_section_iter_get(&iter) != NULL;
		nrf_section_iter_next(&iter))
	{
		handler = nrf_section_iter_get(&iter);
		(*handler)(charging);
	}
}

static inline bool pmic_is_charging(void) {
	return !nrf_gpio_pin_read(PMIC_CHARGE_DETECT_PIN);
}

/*
 * This ISR is common for all external interrupts - that's why we have to
 * keep it in a separate module
 */
void GPIOTE_IRQHandler(void)
{
	if (nrf_gpiote_event_is_set(GPIOTE_CHANNEL_TO_EVENT(GPIOTE_CHARGE_DETECT_CHANNEL))) {
		nrf_gpiote_event_clear(GPIOTE_CHANNEL_TO_EVENT(GPIOTE_CHARGE_DETECT_CHANNEL));
		notify_charger_event_observers(pmic_is_charging());
	}
}

void external_interrupts_init(void)
{
	/* Wakeup if charger is connected */
	nrf_gpio_cfg_sense_input(PMIC_CHARGE_DETECT_PIN, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_SENSE_LOW);

	nrf_gpiote_event_configure(GPIOTE_CHARGE_DETECT_CHANNEL,
			PMIC_CHARGE_DETECT_PIN, NRF_GPIOTE_POLARITY_TOGGLE);
	nrf_gpiote_event_enable(GPIOTE_CHARGE_DETECT_CHANNEL);
	nrf_gpiote_int_enable(GPIOTE_CHANNEL_TO_INT(GPIOTE_CHARGE_DETECT_CHANNEL));

	NVIC_EnableIRQ(GPIOTE_IRQn);
}