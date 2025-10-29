#pragma once
#include "esp_err.h"
#include "esp_err.h"
#include "driver/i2c.h"

// I2C Configuration Variables 
#define I2C_PORT I2C_NUM_0
#define SDA_PIN 8
#define SCL_PIN 9
#define I2C_FREQ_HZ 100000

// MCP3221 device addresses
#define MCP3221_ADDR1 0x49

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t i2c_master_init(void);
esp_err_t mcp3221_read_raw(uint8_t addr, uint16_t *out);

#ifdef __cplusplus
}
#endif