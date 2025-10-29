#include "i2c_driver.h"
#include "driver/i2c.h"
#include "esp_check.h"      

esp_err_t i2c_master_init(void) {
    i2c_config_t cfg = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = SDA_PIN,
        .scl_io_num = SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_DISABLE, // use breakout pull-ups (recommended)
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = I2C_FREQ_HZ,
        .clk_flags = 0
    };
    ESP_RETURN_ON_ERROR(i2c_param_config(I2C_PORT, &cfg), "I2C", "param_config failed");
    ESP_RETURN_ON_ERROR(i2c_driver_install(I2C_PORT, cfg.mode, 0, 0, 0), "I2C", "driver_install failed");
    return ESP_OK;
}

esp_err_t mcp3221_read_raw(uint8_t addr, uint16_t *out) {
    if (!out) return ESP_ERR_INVALID_ARG;

    // MCP3221 is read-only: request 2 bytes
    uint8_t rx[2] = {0};
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    // 7-bit addr -> on-wire {addr, R=1}
    i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, rx, 1, I2C_MASTER_ACK);
    i2c_master_read_byte(cmd, &rx[1], I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    if (err != ESP_OK) return err;

    // 12-bit right-aligned across two bytes: [MSB D11..D4], [LSB D3..D0 | xxxx]
    uint16_t raw = ((uint16_t)rx[0] << 8) | rx[1];
    raw >>= 4;
    *out = raw & 0x0FFF;
    return ESP_OK;
}


