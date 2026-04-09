#pragma once
#include "esp_err.h"
#include "esp_err.h"
#include "driver/i2c.h"

// I2C Configuration Variables 
#define I2C_PORT I2C_NUM_0
#define SDA_PIN 8
#define SCL_PIN 9
#define I2C_FREQ_HZ 300000

// Slave device addresses
// #define MCP3221_ADDR1 0x49
#define ADC_ADDR1 0x50
#define ADC_ADDR2 0x51
#define ADC_ADDR3 0x52
#define ADC_ADDR4 0x54
#define ADC_ADDR5 0x55
#define ADC_ADDR6 0x56
#define ADC_ADDR7 0x58
#define ADC_ADDR8 0x59
#define ADC_ADDR9 0x5A

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t i2c_master_init(void);
esp_err_t mcp3221_read_raw(uint8_t addr, uint16_t *out);

#ifdef __cplusplus
}
#endif