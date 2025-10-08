#pragma once
#include <stdint.h>
#include <stdbool.h>

void fan_init(void);
void fan_arm(void);
void fan_disarm(void);
bool fan_is_armed(void);
void fan_set_target(uint8_t idx, uint16_t duty_0_1000);
void fan_tick_10ms(void); // ramping/slew limiter
