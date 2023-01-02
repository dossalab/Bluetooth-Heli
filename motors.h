#pragma once

#define MOTORS_PWM_MAXVAL	0xFF

void motors_set(unsigned motor1, unsigned motor2);
void motors_init(void);
