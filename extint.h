#pragma once

#include "nrf_section_iter.h"

typedef void (*extint_charger_event_handler)(bool charging);

#define EXTINT_CHARGER_EVENT_SECTION	charger_event_handlers

#define EXTINT_CHARGER_EVENT_HANDLER(_handler) \
	NRF_SECTION_SET_ITEM_REGISTER(EXTINT_CHARGER_EVENT_SECTION, 0, \
		static extint_charger_event_handler const CONCAT_2(_handler, _handler_function)) = (_handler)

void external_interrupts_init(void);
