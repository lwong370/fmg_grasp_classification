#pragma once
#include "esp_err.h"

esp_err_t i2c_master_init(void);
esp_err_t mcp3221_read_raw(uint16_t *out);
