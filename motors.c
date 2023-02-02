#include <nrf_pwm.h>
#include <nrf_gpio.h>
#include "motors.h"

#define MOTORS_PWM		NRF_PWM0
#define MOTORS_PWM_CLOCK	NRF_PWM_CLK_125kHz

/*
 * Only one of the H-bridge halves has PWM on it.
 * Another half is controlled by this GPIO.
 */
#define TAIL_GPIO_PIN		5

/* PWM channel to pin mapping */
#define MOTOR1_PIN		4
#define MOTOR2_PIN		3
#define TAIL_PWM_PIN		6
#define MOTOR1_PWM_CHANNEL	0
#define MOTOR2_PWM_CHANNEL	1
#define TAIL_PWM_CHANNEL	2

static uint32_t motors_pwm_pins[NRF_PWM_CHANNEL_COUNT] = {
	[MOTOR1_PWM_CHANNEL] = MOTOR1_PIN,
	[MOTOR2_PWM_CHANNEL] = MOTOR2_PIN,
	[TAIL_PWM_CHANNEL] = TAIL_PWM_PIN,

	/* The rest is unused... */
	[3 ... NRF_PWM_CHANNEL_COUNT - 1] = NRF_PWM_PIN_NOT_CONNECTED,
};

/* This is the place in memory from which PWM fetches it's values */
static nrf_pwm_values_individual_t motors_pwm_buffer;

/*
 * nrf_pwm.h defines PWM channels as a structure, so we need this ugly macro
 * in order to handle writes by the channel index (which we most likely want to do)
 */
#define write_pwm_buffer(id, value)	({ motors_pwm_buffer.channel_##id = value; })

/*
 * PWM channels are inverted by default, so we need to set MSB
 * for each written value in order to invert them back
 * See datasheet and "nrf_pwm" header for more info
 */
#define PWM_VALUE_INVERT_BIT		BIT_15

#define write_pwm(id, value)		write_pwm_buffer(id, (value) | PWM_VALUE_INVERT_BIT)
#define write_pwm_inverted(id, value)	write_pwm_buffer(id, value)

/*
 * We have to define a PWM sequence even though we just want to use simple PWM.
 * So define it like that - a sequence with a single value in it
 */
static nrf_pwm_sequence_t motors_pwm_sequence = {
	.values.p_individual = &motors_pwm_buffer,
	.length = NRF_PWM_VALUES_LENGTH(motors_pwm_buffer),
	.repeats = 0,
	.end_delay = 0,
};

static void motors_pwm_configure(NRF_PWM_Type *base)
{
	nrf_pwm_pins_set(base, motors_pwm_pins);
	nrf_pwm_sequence_set(base, 0, &motors_pwm_sequence);
	nrf_pwm_decoder_set(base, NRF_PWM_LOAD_INDIVIDUAL, NRF_PWM_STEP_AUTO);
	nrf_pwm_configure(base, MOTORS_PWM_CLOCK, NRF_PWM_MODE_UP, MOTORS_PWM_MAXVAL);
	nrf_pwm_enable(base);
}

void motors_set(uint8_t motor1, uint8_t motor2, int8_t tail)
{
	write_pwm(MOTOR1_PWM_CHANNEL, motor1);
	write_pwm(MOTOR2_PWM_CHANNEL, motor2);

	/* Since tail is signed multiply it by 2 in order to cover the entire PWM range */
	if (tail < 0) {
		write_pwm_inverted(TAIL_PWM_CHANNEL, -2 * tail);
		nrf_gpio_pin_set(TAIL_GPIO_PIN);
	} else {
		write_pwm(TAIL_PWM_CHANNEL, 2 * tail);
		nrf_gpio_pin_clear(TAIL_GPIO_PIN);
	}

	nrf_pwm_task_trigger(MOTORS_PWM, NRF_PWM_TASK_SEQSTART0);
}

void motors_disarm(void)
{
	motors_set(0, 0, 0);
}

void motors_init(void)
{
	nrf_gpio_pin_dir_set(TAIL_GPIO_PIN, NRF_GPIO_PIN_DIR_OUTPUT);
	motors_pwm_configure(MOTORS_PWM);

	motors_disarm();
}
