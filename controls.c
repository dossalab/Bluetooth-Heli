#include <app_error.h>
#include <stdlib.h>

#include "ble.h"
#include "motors.h"
#include "resource-map.h"

/* Ignore values lower than that so it's easier to disarm */
#define MOTOR_LOW_THRESHOLD		15
#define RUDDER_SENSITIVITY_PRESCALER	4

typedef struct {
	uint8_t throttle;
	int8_t rudder;
	int8_t tail;

	uint8_t reserved;
} control_packet;

static ble_gatts_char_handles_t controls_char_handle;

static inline void motor_pwm_range_check(int *value) {
	if (*value < MOTOR_LOW_THRESHOLD) {
		*value = 0;
	}

	if (*value > MOTORS_PWM_MAXVAL) {
		*value = MOTORS_PWM_MAXVAL;
	}
}

static void handle_controls_packet(control_packet *packet)
{
	int motor1, motor2, tail;

	motor1 = motor2 = packet->throttle;
	tail = packet->tail;

	motor1 += packet->rudder / RUDDER_SENSITIVITY_PRESCALER;
	motor2 -= packet->rudder / RUDDER_SENSITIVITY_PRESCALER;

	motor_pwm_range_check(&motor1);
	motor_pwm_range_check(&motor2);

	if (abs(tail) < MOTOR_LOW_THRESHOLD) {
		tail = 0;
	}

	motors_set(motor1, motor2, tail);
}

static void handle_gatts_write(const ble_gatts_evt_write_t *event)
{
	if (event->handle == controls_char_handle.value_handle
			&& event->len == sizeof(control_packet)) {
		control_packet *packet = (void *)event->data;
		handle_controls_packet(packet);
	}
}

static void ble_events_handler(const ble_evt_t *event, void *user)
{
	switch (event->header.evt_id) {
	case BLE_GATTS_EVT_WRITE:
		handle_gatts_write(&event->evt.gatts_evt.params.write);
		break;
	case BLE_GAP_EVT_DISCONNECTED:
		motors_disarm();
		break;
	}
}

NRF_SDH_BLE_OBSERVER(controls_service_observer, BLE_C_OBSERVERS_PRIORITY, ble_events_handler, NULL);

static void controls_service_init(unsigned base_uuid)
{
	ret_code_t err;
	uint16_t service_handle;

	ble_add_char_params_t controls_char_params = {
		.uuid             = CONTROLS_CHAR_UUID,
		.max_len          = sizeof(control_packet),
		.init_len         = 0,
		.p_init_value     = NULL,
		.char_props.read  = 1,
		.char_props.write = 1,
		.read_access      = SEC_OPEN,
		.write_access     = SEC_OPEN,
	};

	service_handle = ble_c_create_service(base_uuid, CONTROLS_SERVICE_UUID);

	err = characteristic_add(service_handle, &controls_char_params, &controls_char_handle);
	APP_ERROR_CHECK(err);
}

void controls_init(unsigned base_uuid)
{
	controls_service_init(base_uuid);
}
