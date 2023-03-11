#pragma once

/*
* This typically must be lower than SoftDevice IRQ priorities, otherwise we'll get
* stuck while caling Bluetooth-related APIs from the IRQ context
*/
#define DEFAULT_IRQ_PRIORITY	6
#define ADC_IRQ_PRIORITY	DEFAULT_IRQ_PRIORITY