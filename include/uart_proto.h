#pragma once
#include <stdint.h>
#include <stdbool.h>

void uart_proto_init(void);
void uart_proto_poll(void); // call in main loop
void uart_proto_printf(const char *fmt, ...);
