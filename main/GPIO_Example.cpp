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
#include "../liblinear/linear.h"
#include "esp_log.h"
#include "esp_spiffs.h"
// #include "driver/adc.h"  
#include "esp_adc/adc_oneshot.h"
// #include "../src/fsr_reader.h"
#include "predict.h"
// #include "bluetooth_ble.h"

#define TAG "MY_APP"
#define NUM_FEATURES 8

#define NUM_FSRS 8
const adc_channel_t fsr_pins[NUM_FSRS] = {
    ADC_CHANNEL_0, 
    ADC_CHANNEL_1, 
    ADC_CHANNEL_2, 
    ADC_CHANNEL_3, 
    ADC_CHANNEL_5, 
    ADC_CHANNEL_6, 
    ADC_CHANNEL_7, 
    ADC_CHANNEL_8
};
#define SAMPLES 100

std::map<int, std::string> label_map = {
    {0, "CR"},
    {1, "CW"},
    {2, "IF"},
    {3, "IP"},
    {4, "KP"},
    {5, "PP"},
    {6, "TP"},
    {7, "WE"},
    {8, "WF"},
    {9, "WR"}
};

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

        // MULTIPLE
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
    
        while (1) {        
            for (int i = 0; i < NUM_FSRS; i++) {
                int raw_value;
                esp_err_t err = adc_oneshot_read(adc1_handle, fsr_pins[i], &raw_value);
                if (err == ESP_OK) {
                    printf("FSR%d: %d\n", i, raw_value);  // Print one FSR reading per line
                } else {
                    printf("FSR%d: ADC Read Failed (%d)\n", i, err);
                }
            }
            printf("----------------------\n"); 
            vTaskDelay(pdMS_TO_TICKS(500));  // wait 500ms between batches
        }
    adc_oneshot_del_unit(adc1_handle);
}

extern "C" void app_main(void) {
    // FSR CODE
    xTaskCreate(&read_fsr_task, "read_fsr_task", 4096, NULL, 5, NULL);
    
    double input_vector[8] = {0.0024629415540150463, 1.1628113507955182, 0.9188483074903218, 0.009111933349554445, 0.0043415847116466426, 0.013750822148270907, 0.5829985317962872, 0.016803155103287937
    }; // Example input for 8 features
    double input_scaled[NUM_FEATURES];

    normalize(input_vector, input_scaled);
    int prediction = predict(input_scaled);
    std::string predicted_label = label_map[prediction]; 
    ESP_LOGI(TAG, "Prediction: %d (%s)", prediction, predicted_label.c_str());

    // ble_init();
    // xTaskCreate(ble_notify_task, "fmg_notify", 4096, NULL, 5, NULL);
}
