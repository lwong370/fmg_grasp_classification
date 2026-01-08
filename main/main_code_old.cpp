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
#include <string>
#include <array>
#include "../liblinear/linear.h"
#include "esp_log.h"
// #include "esp_spiffs.h"
#include "esp_adc/adc_oneshot.h"
// #include "../src/fsr_reader.h"
// #include "driver/adc.h"  
#include "predict.h"
#include "driver/i2c.h"
#include "i2c_driver.h"
#include "../components/constants/types.h"
#include "../components/feature_extraction/feature_extraction.h"
#include "../components/constants/config.h"

extern "C" { 
    #include "nimble_ble.h"
    #include "esp_err.h"
    #include "esp_timer.h"
    #include "esp_vfs_dev.h" 
    #include "driver/usb_serial_jtag.h"

    #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,1,0)
        #include "esp_vfs_dev.h"
        #define USE_USB_STDIO()  esp_vfs_dev_usb_serial_jtag_use_driver()
    #else
        #include "esp_vfs_usb_serial_jtag.h"
        #define USE_USB_STDIO()  esp_vfs_usb_serial_jtag_use_driver()
    #endif
}

#define TAG "MY_APP"
QueueHandle_t feature_queue;
int fsr_values[NUM_FSRS];  

const adc_channel_t fsr_pins[NUM_FSRS] = {
    //ADC_CHANNEL_0, 
    ADC_CHANNEL_1, 
    ADC_CHANNEL_2,
    ADC_CHANNEL_3,
    ADC_CHANNEL_5, 
    ADC_CHANNEL_6 
    // ADC_CHANNEL_7
    // ADC_CHANNEL_8
};

static const uint8_t ADC_SLAVE_ADDRS[] = {
    MCP3221_ADDR1, ADC_ADDR1, ADC_ADDR2, ADC_ADDR3, ADC_ADDR4
};

void read_fsr_task(void *pvParameter) {
    int log_counter = 0;
    const int log_interval = 100;
    
    const TickType_t sample_period = pdMS_TO_TICKS(20); // 50 Hz loop period 
    TickType_t last_wake = xTaskGetTickCount();
    
    // --- Initialize ADC handle and configuration for ADC one-shot mode ---
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,  // Default clock source
        .ulp_mode = ADC_ULP_MODE_DISABLE    // No ULP mode
    };
    adc_oneshot_new_unit(&init_config, &adc1_handle);

    // Configure the ADC channels for the FSR sensor pins
    for (int i = 0; i < NUM_FSRS; i++) {
        adc_oneshot_chan_cfg_t channel_config = {
            .atten = ADC_ATTEN_DB_12,  // 12dB attenuation (0-3.3V range)
            .bitwidth = ADC_BITWIDTH_DEFAULT   // 12-bit resolution
        };
        esp_err_t err = adc_oneshot_config_channel(adc1_handle, fsr_pins[i], &channel_config);
        if (err != ESP_OK) {
            printf("Failed to configure ADC channel %d: %d\n", fsr_pins[i], err);
            adc_oneshot_del_unit(adc1_handle);  // Cleanup before exiting
            vTaskDelete(NULL);
        }
    }

    // Holds the last sample of size WINDOW_SIZE for each FSR sensor
    static int signal_buffer[NUM_FSRS][WINDOW_SIZE] = {0};
    ESP_LOGI(TAG, "Using static buffer: %d bytes", sizeof(signal_buffer));
    
    // Index to store next sample in circular buffer
    int buffer_index = 0;

    int decim = 0;

    while (1) {  

        // Read a new sample from each FSR     
        for (int i = 0; i < NUM_FSRS; i++) {
            int raw_value = 0;
            esp_err_t err = adc_oneshot_read(adc1_handle, fsr_pins[i], &raw_value);
            if (err == ESP_OK) {
                fsr_values[i] = raw_value;                       // collect for BLE
                signal_buffer[i][buffer_index] = raw_value;      // ring buffer   
            } else {
                printf("FSR%d: ADC Read Failed (%d)\n", i, err);
                fsr_values[i] = 0;                               // keep packet defined
            }
            esp_rom_delay_us(40); // slow down ADC sampling rate
        }

        // Print FSR readings to console
        if (++decim >= 25) {
            //printf("%d\n", fsr_values[0]);
            printf("%d, %d, %d\n", fsr_values[0], fsr_values[1], fsr_values[2]);
            decim = 0;
        }

        if (ble_notify_ready()) {
            // Send one notification containing all channels
            bool ok = ble_send_fsr_sample(fsr_values, NUM_FSRS);  

            if (!ok) {
                ESP_LOGW(TAG, "enqueue failed (queue null/full or n invalid)");
            } else {
                ESP_LOGW(TAG, "queued success");
            }
        }

        vTaskDelayUntil(&last_wake, sample_period);
        
        log_counter = (log_counter + 1) % log_interval;
    }
    
    adc_oneshot_del_unit(adc1_handle);
    vTaskDelete(NULL);
}

static inline float code_to_volts(uint16_t code, float vref) {
    return (code / 4095.0f) * vref;
}

void i2c_read_sensors(void *pvParameter) {
    const float VREF = 3.3f; 
    while (1) {
        for (size_t i = 0; i < sizeof(NUM_FSRS); ++i) {
            const uint8_t addr = ADC_SLAVE_ADDRS[i];
            uint16_t code = 0;
            esp_err_t e = mcp3221_read_raw(addr, &code);
            if (e == ESP_OK) {
                float v = code_to_volts(code, VREF);
                fsr_values[i] = code;     
                ESP_LOGI(TAG, "MCP3221[0x%02X] code=%4u  V=%.3f", addr, code, v);
            } else {
                ESP_LOGW(TAG, "Read fail @ 0x%02X: %s", addr, esp_err_to_name(e));
            }
        }

        // For sending data over BlueTooth 
        if (ble_notify_ready()) {
            // Send one notification containing all channels
            bool ok = ble_send_fsr_sample(fsr_values, NUM_FSRS);  

            if (!ok) {
                ESP_LOGW(TAG, "enqueue failed (queue null/full or n invalid)");
            } else {
                ESP_LOGW(TAG, "queued success");
            }
        }

        // For sending data over direct USB
        // uint64_t t_us = (uint64_t)esp_timer_get_time();
        // printf("%" PRIu64, t_us);
        // for (size_t i = 0; i < NUM_FSRS; ++i) {
        //     printf(",%u", (unsigned)fsr_values[i]);
        // }
        // printf("\n");

        // vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void init_usb_stdio(void) {
    usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&cfg));
    esp_vfs_usb_serial_jtag_use_driver();
    setvbuf(stdout, nullptr, _IONBF, 0);  // unbuffered printf
}

void i2c_scan() {
    int found = 0;
    for (uint8_t addr = 1; addr < 127; ++addr) { // I2C addresses are 7-bits
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true); // Shifts address to left by 1 bit, making room for R/W bit
        i2c_master_stop(cmd);
        esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);
        if (err == ESP_OK) {  // Check and logs if ACK sent back
            ESP_LOGI("SCAN", "Found @ 0x%02X", addr); 
            found++; } 
    }
    ESP_LOGI("SCAN", "Found %d device(s).", found);
}

extern "C" void app_main(void) {
    ble_init();
    
    // Read FSR data to analog pins and run BT
    //xTaskCreate(&read_fsr_task, "read_fsr_task", 4096, NULL, 5, NULL);

    // Enable I2C
    ESP_ERROR_CHECK(i2c_master_init());
    i2c_scan();

    // Direct usb data sending
    init_usb_stdio();

    xTaskCreate(&i2c_read_sensors, "i2c_read_sensors", 4096, NULL, 5, NULL);
    //uint64_t t_us = esp_timer_get_time();

    // Task delay
    vTaskDelay(pdMS_TO_TICKS(1000));
}
