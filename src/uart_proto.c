#include "stm32f7xx_hal.h"
#include "pins.h"
#include "uart_proto.h"
#include "fan.h"
#include "scd41.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

extern UART_HandleTypeDef huart2;
static char rxbuf[128];
static size_t rxlen = 0;

void uart_proto_printf(const char *fmt, ...)
{
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) {
        HAL_UART_Transmit(&huart2, (uint8_t*)buf, (uint16_t)n, 100);
    }
}

static int ci_equal(const char *a, const char *b)
{
    while (*a && *b) {
        char ca = (char)toupper((unsigned char)*a);
        char cb = (char)toupper((unsigned char)*b);
        if (ca != cb) return 0;
        ++a; ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int ci_startswith(const char *a, const char *prefix)
{
    while (*a && *prefix) {
        char ca = (char)toupper((unsigned char)*a);
        char cb = (char)toupper((unsigned char)*prefix);
        if (ca != cb) return 0;
        ++a; ++prefix;
    }
    return *prefix == '\0';
}

static void trim(char *s)
{
    size_t n = strlen(s);
    while (n && (s[n-1] == '\r' || s[n-1] == '\n' || isspace((unsigned char)s[n-1]))) { s[--n] = 0; }
    while (*s && isspace((unsigned char)*s)) memmove(s, s+1, strlen(s));
}

static void handle_line(char *line)
{
    trim(line);
    if (!*line) return;

    if (ci_equal(line, "STATUS")) {
        uart_proto_printf("ARMED=%d\r\n", fan_is_armed());
        return;
    }
    if (ci_equal(line, "ARM")) {
        fan_arm();
        uart_proto_printf("OK ARM\r\n");
        return;
    }
    if (ci_equal(line, "DISARM")) {
        fan_disarm();
        uart_proto_printf("OK DISARM\r\n");
        return;
    }
    if (ci_startswith(line, "SET ")) {
        unsigned idx = 0, duty = 0;
        if (sscanf(line+4, "%u %u", &idx, &duty) == 2 && idx < 4 && duty <= 1000) {
            fan_set_target((uint8_t)idx, (uint16_t)duty);
            uart_proto_printf("OK SET %u %u\r\n", idx, duty);
        } else {
            uart_proto_printf("ERR SET usage: SET <0..3> <0..1000>\r\n");
        }
        return;
    }
    if (ci_equal(line, "GET SENSORS")) {
#if SCD41_ENABLE
        scd41_sample_t s;
        if (scd41_read_measurement(&s) && s.valid) {
            uart_proto_printf("CO2=%u ppm T=%d.%02d C RH=%u.%02u %%\r\n",
                s.co2_ppm, s.temp_c_x100/100, (int)(s.temp_c_x100%100),
                s.rh_x100/100, s.rh_x100%100);
        } else {
            uart_proto_printf("NODATA\r\n");
        }
#else
        uart_proto_printf("SCD41 DISABLED\r\n");
#endif
        return;
    }

    uart_proto_printf("ERR UNKNOWN\r\n");
}

void uart_proto_init(void)
{
    rxlen = 0;
    uart_proto_printf("CLI ready. Commands: STATUS, ARM, DISARM, SET i val, GET SENSORS\r\n");
}

void uart_proto_poll(void)
{
    uint8_t ch;
    while (HAL_UART_Receive(&huart2, &ch, 1, 0) == HAL_OK) {
        if (ch == '\n' || ch == '\r') {
            rxbuf[rxlen] = 0;
            handle_line(rxbuf);
            rxlen = 0;
        } else if (rxlen + 1 < sizeof(rxbuf)) {
            rxbuf[rxlen++] = (char)ch;
        }
    }
}