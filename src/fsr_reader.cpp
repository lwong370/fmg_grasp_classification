#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include "../liblinear/linear.h"
#define TAG "MY_APP"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "driver/adc.h"  
#include "esp_adc/adc_oneshot.h"
#include "../src/fsr_reader.h"

#define FSR_PIN ADC_CHANNEL_7 // Change this based on your ESP32-S3 board
#define SAMPLES 100

static const char *TAG = "FSR_SENSOR";

// FSR CODE
void read_fsr_task(void *pvParameter) {
    // Initialize ADC handle and configuration for ADC one-shot mode
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,  // Default clock source
        .ulp_mode = ADC_ULP_MODE_DISABLE    // No ULP mode
    };

    // Initialize ADC one-shot unit
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    // Configure the ADC channel for the FSR sensor pin
    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,  // 12dB attenuation (0-3.3V range)
        .bitwidth = ADC_BITWIDTH_DEFAULT   // 12-bit resolution
    };
    esp_err_t err = adc_oneshot_config_channel(adc1_handle, FSR_PIN, &channel_config);
    if (err != ESP_OK) {
        printf("Failed to configure ADC channel: %d\n", err);
        adc_oneshot_del_unit(adc1_handle);  // Cleanup before exiting
        vTaskDelete(NULL);
    }

    while (1) {
        // Read the raw ADC value
        int raw_value;
        esp_err_t err = adc_oneshot_read(adc1_handle, FSR_PIN, &raw_value);
        if (err == ESP_OK) {
            printf("FSR Reading: %d\n", raw_value);
        } else {
            printf("ADC Read Failed: %d\n", err);
        }
        vTaskDelay(pdMS_TO_TICKS(100));  // Delay for 100ms (adjust as needed)
    }

    adc_oneshot_del_unit(adc1_handle);
}