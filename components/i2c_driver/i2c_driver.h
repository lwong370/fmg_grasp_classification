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

/**
 * @brief Initialize the I2C master peripheral.
 *
 * Configures the I2C bus with the pin assignments and clock speed
 * defined in config.h, then installs the ESP-IDF I2C driver.
 * Needs to be called once before I2C read or scan operations.
 *
 * @return ESP_OK on success, or an esp_err_t error code on failure.
 */
esp_err_t i2c_master_init(void);


/**
 * @brief Scan the I2C bus for connected devices and populate
 *        detected_slave_addresses with any found addresses.
 *
 * @param slave_addrs  Array to store 7-bit slave device addresses found
 * @param slave_count  Set to the number of devices found
 * @param max          Max number of slave_addrs allowed
 */
void i2c_scan(uint8_t *slave_addrs, int *slave_count, int max);


/**
 * @brief Read raw 12-bit conversion result from ADC121c021 components 
 * of slave devices over I2C.
 *
 * Sends the device address with the read bit set, then reads 2 bytes
 * (MSB then LSB) and assembles them into a single 12-bit value (0–4095).
 * ACK is sent after the first byte to continue the read; NACK is sent
 * after the second byte to signal end of transaction.
 *
 * @param addr  7-bit I2C address of the slave devices
 * @param out   Output pointer; receives the raw ADC code (0–4095)
 * @return      ESP_OK on success, or esp_err_t error on failure
 */
esp_err_t read_raw(uint8_t addr, uint16_t *out);

#ifdef __cplusplus
}
#endif