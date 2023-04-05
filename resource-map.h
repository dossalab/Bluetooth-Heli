#pragma once

#define PPI_ADC_CHANNEL			0
#define PPI_LED_CHANNEL			1
#define PPI_RTC_COMPARE_CLEAR_CHANNEL	2

#define GPIOTE_LED_CHANNEL		0
#define GPIOTE_CHARGE_DETECT_CHANNEL	1

/*
 * Battery gauge - TI's BQ27427 connected to TWI master
 * Interrupt pin is used to deliver notifications (e.g state-of-charge changes)
 */
#define BATTERY_GAUGE_PIN_INT		6
#define BATTERY_GAUGE_PIN_TWI_SDA	7
#define BATTERY_GAUGE_PIN_TWI_SCL	8

/* Make sure to update all of the following params while switching TWI instances */
#define BATTERY_GAUGE_TWI		NRF_TWIM0
#define BATTERY_GAUGE_TWI_NVIC_IRQN	SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQn
#define BATTERY_GAUGE_TWI_IRQ_HANDLER	SPIM0_SPIS0_TWIM0_TWIS0_SPI0_TWI0_IRQHandler
