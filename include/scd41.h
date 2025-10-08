#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t co2_ppm;     // ppm
    int16_t  temp_c_x100; // °C * 100
    uint16_t rh_x100;     // %RH * 100
    bool     valid;
} scd41_sample_t;

bool scd41_init(void);
bool scd41_start_periodic(void);
bool scd41_read_measurement(scd41_sample_t *out); // non-blocking poll, true if new data decoded
