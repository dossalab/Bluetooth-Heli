#pragma once

#define PPI_ADC_CHANNEL			0
#define PPI_LED_CHANNEL			1
#define PPI_RTC_COMPARE_CLEAR_CHANNEL	2

#define GPIOTE_LED_CHANNEL		0
#define GPIOTE_CHARGE_DETECT_CHANNEL	1

/*
 * A single LED is used for the notifications
 */

#define LED_PIN				0

/*
 * Motors are controlled using one of the PWM instances
 */
#define MOTORS_PWM			NRF_PWM0
#define MOTOR1_PIN			2
#define MOTOR2_PIN			1
#define TAIL_PWM_PIN			3

/*
 * Only one of the H-bridge halves has PWM on it.
 * Another half is controlled by this GPIO.
 */
#define TAIL_GPIO_PIN			4

/*
 * ENC-03 represents angular speed as a voltage output relative to its VREF,
 * Luckily for us NRF52's ADC has differential inputs, so we can directly measure
 * the signal relative to the reference.
 */
#define GYRO_POWER_ENABLE_PIN		26
#define GYRO_ADC_MEASUREMENT_INPUT	NRF_SAADC_INPUT_AIN4
#define GYRO_ADC_REFERENCE_INPUT	NRF_SAADC_INPUT_AIN5

/*
 * Those pins are connected to nPM1100 and may be used to sense the
 * battery failure and charger status
 *
 * Both of those are open drain and require pullups (hence the value is inverted)
 */
#define PMIC_CHARGE_DETECT_PIN		11
#define PMIC_FAILURE_DETECT_PIN		12

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
