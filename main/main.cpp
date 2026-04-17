#include <stdio.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <map>
#include <string>
#include <array>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_log.h"
#include "driver/usb_serial_jtag.h"
#include "driver/i2c.h"
#include "i2c_driver.h"
#include "../components/constants/types.h"
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

int num_sensor_chls = 0;
int sampleIndex = 0;
uint8_t detected_slave_addresses[MAX_SENSOR_CHANNELS];

QueueHandle_t feature_queue;
static QueueHandle_t usb_queue = NULL;

static inline float code_to_volts(uint16_t code, float vref) {
    return (code / 4095.0f) * vref;
}

void i2c_scan() {
    int found = 0;  // number of slave devices discovered on I2C
    for (uint8_t addr = 1; addr < 127; ++addr) { // I2C addresses are 7-bits
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true); // Shifts address to left by 1 bit, making room for R/W bit
        i2c_master_stop(cmd);
        esp_err_t err = i2c_master_cmd_begin(I2C_PORT, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);
        if (err == ESP_OK) {  // Check and logs if ACK sent back
            ESP_LOGI("SCAN", "Found @ 0x%02X", addr); 
            detected_slave_addresses[found] = addr;
            found++; 
        } 
    }
    num_sensor_chls = found;
    ESP_LOGI("SCAN", "Found %d device(s).", found);
}

void i2c_read_sensors(void *pvParameter) {
    const float VREF = 3.3f; 
    while (1) {
        sensor_sample_t sample = {0};
        sample.index = sampleIndex;
        sampleIndex++;
        for (size_t i = 0; i < num_sensor_chls; ++i) {
            const uint8_t addr = detected_slave_addresses[i];
            uint16_t code = 0;
            esp_err_t e = mcp3221_read_raw(addr, &code);
            if (e == ESP_OK) {
                float v = code_to_volts(code, VREF);
                sample.ch[i].addr = addr;
                sample.ch[i].data = code;
                // ESP_LOGI(TAG, "MCP3221[0x%02X] code=%4u  V=%.3f", addr, code, v);
            } else {
                ESP_LOGW(TAG, "Read fail @ 0x%02X: %s", addr, esp_err_to_name(e));
            }
        }

        // Check if USB cable plugged in
        if (usb_serial_jtag_is_connected()) {
            // For sending data over direct USB
            xQueueOverwrite(usb_queue, &sample);
        } else {
            // Pipeline for sending data over Bluetooth 
            if (ble_notify_ready()) {
                ble_send_fsr_sample(sample.ch, num_sensor_chls);  // Send one notification with all channels
            }
        }
 
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void init_usb_stdio(void) {
    usb_serial_jtag_driver_config_t cfg = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&cfg));
    esp_vfs_usb_serial_jtag_use_driver();
    setvbuf(stdout, nullptr, _IONBF, 0);  // unbuffered printf
}

static void usb_print_csv_sample(int curr_index, sensor_x *fsr, size_t num_channels) {
    char buffer[128]; 
    int n = 0;
    bool truncated = false;

    n += snprintf(buffer + n, sizeof(buffer) - n, "%d", curr_index);  // Write timestamp
    for (size_t i = 0; i < num_channels; ++i) {
        if(n < (int)sizeof(buffer)) {
            n += snprintf(buffer + n, sizeof(buffer) - n, ", 0x%02X: %u", fsr[i].addr, (unsigned)fsr[i].data);
        }
    }

    if (n < (int)sizeof(buffer)) {
        n += snprintf(buffer + n, sizeof(buffer) - n, "\n");
    } else {
        truncated = true;
    }

    if(truncated) {
        static uint32_t trunc_count = 0;
        trunc_count+=1;
        if((trunc_count % 100) == 1) {
            ESP_LOGW(TAG, "USB data line truncated. Buffer too small for incoming data.");
        }
        return;
    }

    printf("%s", buffer);
}

static void usb_send_binary_sample(int curr_index, sensor_x *fsr, size_t num_channels) {
    sensor_sample_t pkt = {};
    pkt.index = curr_index;

    for (size_t i = 0; i < num_channels && i < MAX_SENSOR_CHANNELS; ++i) {
        pkt.ch[i].addr = fsr[i].addr;
        pkt.ch[i].data = fsr[i].data;
    }

    const uint8_t *buf = (const uint8_t *)&pkt;
    size_t total = sizeof(pkt);
    size_t sent = 0;

    while (sent < total) {
        int n = usb_serial_jtag_write_bytes(
            (const char *)(buf + sent),
            total - sent,
            pdMS_TO_TICKS(100)
        );

        if (n > 0) {
            sent += (size_t)n;
        } else {
            break;
        }
    }
}

// static void usb_send_binary_sample(int curr_index, sensor_x *fsr, size_t num_channels) {
//     sensor_sample_t pkt = {};
//     pkt.index = (uint32_t)curr_index;

//     for (int i = 0; i < num_channels && i < MAX_SENSOR_CHANNELS; ++i) {
//         pkt.ch[i].addr = fsr[i].addr;
//         pkt.ch[i].data = (uint16_t)fsr[i].data;
//     }

//     //usb_serial_jtag_write_bytes((const char *)&pkt, sizeof(pkt), 0);
//     const uint8_t *buf = (const uint8_t *)&pkt;
//     size_t total = sizeof(pkt);
//     size_t sent = 0;

//     while (sent < total) {
//         int n = usb_serial_jtag_write_bytes((const char *)(buf + sent), total - sent, pdMS_TO_TICKS(100));
//         if (n > 0) {
//             sent += (size_t)n;
//         } else {
//             break;
//         }
//     }

//     //ESP_LOGW(TAG, "Test1"); 
//    // if (written != (int)sizeof(pkt)) {
//         static uint32_t short_write_count = 0;
//         // ESP_LOGW(TAG, "Test2"); 
//         //short_write_count++;
//         // if ((short_write_count % 100) == 1) {
//         //     ESP_LOGW(TAG, "USB short write: wrote %d of %u bytes",
//         //              written, (unsigned)sizeof(pkt));
//         // }
//    // }
// }

static void usb_send_task(void *pv) {
    sensor_sample_t sample;
    
    while(1) {
        if (xQueueReceive(usb_queue, &sample, portMAX_DELAY) != pdTRUE) continue;

        if (usb_serial_jtag_is_connected()) {
            usb_send_binary_sample(sample.index, sample.ch, num_sensor_chls);
        }
    }
}


void log_heap(void) {
    ESP_LOGI(TAG, "free heap (8-bit) = %u", (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT));
    ESP_LOGI(TAG, "min free heap (8-bit) = %u", (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
    ESP_LOGI(TAG, "largest free block (8-bit) = %u", (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
}

extern "C" void app_main(void) {
    
    // Initialize Bluetooth
    ble_init();

    // Enable I2C
    ESP_ERROR_CHECK(i2c_master_init());
    i2c_scan();  
    
    // Initialize USB and make USB queue
    init_usb_stdio();
    usb_queue = xQueueCreate(1, sizeof(sensor_sample_t));   
    configASSERT(usb_queue);
    xTaskCreate(usb_send_task, "usb_send", 4096, NULL, 4, NULL);

    // Create I2C sensors
    xTaskCreate(&i2c_read_sensors, "i2c_read_sensors", 4096, NULL, 5, NULL);

    log_heap();

    // Task delay
    vTaskDelay(pdMS_TO_TICKS(1000));
}
