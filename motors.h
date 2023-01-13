#pragma once

#include <stdint.h>

#define MOTORS_PWM_MAXVAL	0xFF

void motors_set(uint8_t motor1, uint8_t motor2, int8_t tail);
void motors_disarm(void);

void motors_init(void);
