#include <nrf_pwm.h>
#include "motors.h"

/*
 * PWM channels are inverted by default, so we need to set MSB
 * for each written value in order to invert them back
 * See datasheet and "nrf_pwm" header for more info
 */
#define PWM_VALUE_INVERT_BIT	BIT_15

#define MOTORS_PWM		NRF_PWM0
#define MOTORS_PWM_CLOCK	NRF_PWM_CLK_125kHz

/* PWM channel to pin mapping */
#define MOTOR1_PIN		4
#define MOTOR2_PIN		3
#define MOTOR1_PWM_CHANNEL	0
#define MOTOR2_PWM_CHANNEL	1

static uint32_t motors_pwm_pins[NRF_PWM_CHANNEL_COUNT] = {
	[MOTOR1_PWM_CHANNEL] = MOTOR1_PIN,
	[MOTOR2_PWM_CHANNEL] = MOTOR2_PIN,

	/* The rest is unused... */
	[2 ... NRF_PWM_CHANNEL_COUNT - 1] = NRF_PWM_PIN_NOT_CONNECTED,
};

/* This is the place in memory from which PWM fetches it's values */
static nrf_pwm_values_individual_t motors_pwm_buffer;

static inline void motors_pwm_write_buffer(uint16_t *ptr, uint16_t val) {
	if (val > MOTORS_PWM_MAXVAL) {
		val = MOTORS_PWM_MAXVAL;
	}

	*ptr = val | PWM_VALUE_INVERT_BIT;
}

/*
 *nrf_pwm.h defines PWM channels as a structure, so we need this ugly macro
 * in order to handle writes by the channel index (which we most likely want to do)
 */
#define PWM_CHANNEL_BUFFER(id)	&(motors_pwm_buffer.channel_##id)
#define motors_write_pwm(channel_id, val) \
		motors_pwm_write_buffer(PWM_CHANNEL_BUFFER(channel_id), val)

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

void motors_set(unsigned motor1, unsigned motor2)
{
	motors_write_pwm(MOTOR1_PWM_CHANNEL, motor1);
	motors_write_pwm(MOTOR2_PWM_CHANNEL, motor2);

	nrf_pwm_task_trigger(MOTORS_PWM, NRF_PWM_TASK_SEQSTART0);
}

void motors_init(void)
{
	motors_pwm_configure(MOTORS_PWM);
	motors_set(0, 0);
}
