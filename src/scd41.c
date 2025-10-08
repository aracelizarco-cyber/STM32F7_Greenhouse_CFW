#include "stm32f7xx_hal.h"
#include "pins.h"
#include "fan.h"
#include "scd41.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;

#define SCD41_ADDR     (0x62 << 1) // 7-bit 0x62 -> 8-bit address

static uint8_t crc8(const uint8_t *data, int len)
{
    uint8_t crc = 0xFF;
    for (int i=0;i<len;i++) {
        crc ^= data[i];
        for (int b=0;b<8;b++) {
            if (crc & 0x80) crc = (uint8_t)((crc << 1) ^ 0x31);
            else crc <<= 1;
        }
    }
    return crc;
}

static HAL_StatusTypeDef scd41_write_cmd(uint16_t cmd)
{
    uint8_t buf[2] = { (uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xFF) };
    return HAL_I2C_Master_Transmit(&hi2c1, SCD41_ADDR, buf, 2, 100);
}

static HAL_StatusTypeDef scd41_read_block(uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Master_Receive(&hi2c1, SCD41_ADDR, buf, len, 100);
}

bool scd41_init(void)
{
    // Soft reset (0x3639) optional
    scd41_write_cmd(0x3639);
    HAL_Delay(20);
    // Stop any prior periodic measurement
    scd41_write_cmd(0x3F86);
    HAL_Delay(1);
    return true;
}

bool scd41_start_periodic(void)
{
    // Start periodic measurement (0x21B1)
    if (scd41_write_cmd(0x21B1) != HAL_OK) return false;
    // First valid measurement after ~5s
    return true;
}

// Returns true if valid decoded sample present
bool scd41_read_measurement(scd41_sample_t *out)
{
    if (!out) return false;
    memset(out, 0, sizeof(*out));

    // Read measurement (0xEC05). Then read 9 bytes: CO2 MSB LSB CRC, T MSB LSB CRC, RH MSB LSB CRC
    if (scd41_write_cmd(0xEC05) != HAL_OK) return false;
    HAL_Delay(5); // datasheet recommends small wait

    uint8_t raw[9] = {0};
    if (scd41_read_block(raw, sizeof(raw)) != HAL_OK) return false;

    // Verify CRCs
    if (crc8(&raw[0],2) != raw[2]) return false;
    if (crc8(&raw[3],2) != raw[5]) return false;
    if (crc8(&raw[6],2) != raw[8]) return false;

    uint16_t co2_raw = (uint16_t)((raw[0] << 8) | raw[1]);
    uint16_t t_raw   = (uint16_t)((raw[3] << 8) | raw[4]);
    uint16_t rh_raw  = (uint16_t)((raw[6] << 8) | raw[7]);

    // Convert per Sensirion formula
    // T(°C) = -45 + 175 * t_raw / 65535
    // RH(%) = 100 * rh_raw / 65535
    int32_t t_c_x100 = (int32_t)((17500LL * t_raw) / 65535) - 4500;
    uint16_t rh_x100 = (uint16_t)((10000UL * rh_raw) / 65535UL);

    out->co2_ppm     = co2_raw;
    out->temp_c_x100 = (int16_t)t_c_x100;
    out->rh_x100     = rh_x100;
    out->valid       = true;
    return true;
}