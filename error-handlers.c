#include <nrf_soc.h>
#include "motors.h"

__STATIC_FORCEINLINE void error_handler(void)
{
	__disable_irq();
	motors_disarm();

	while(1);
}

void app_error_fault_handler(uint32_t id, uint32_t pc, uint32_t _info)
{
	error_handler();
}

void HardFault_Handler(void)
{
	error_handler();
}

