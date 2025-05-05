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
#include "feature_extraction.h"
// #include "bluetooth_ble.h"

#define TAG "MY_APP"

// FSR & Sampling Parameters
#define NUM_FSRS 8
#define SAMPLE_RATE_MS 10           // Sample every 10ms
#define WINDOW_DURATION_MS 200
#define STEP_DURATION_MS 50
#define WINDOW_SIZE (WINDOW_DURATION_MS / SAMPLE_RATE_MS)  // 20 samples
#define STEP_SIZE (STEP_DURATION_MS / SAMPLE_RATE_MS)      // 5 samples
#define NUM_FEATURES 8
#define FEATURE_QUEUE_LENGTH 5

using MAVFeature = std::vector<float>;
using Window = std::vector<std::vector<int>>;
QueueHandle_t feature_queue;
constexpr int NUM_CHANNELS = 8;

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

    // --- Circular buffer for each FSR Channel ---
    // Holds the last sample of size WINDOW_SIZE for each FSR sensor
    int signal_buffer[NUM_FSRS][WINDOW_SIZE] = {0};
    
    // Index to store next sample in circular buffer
    int buffer_index = 0;

    // Counts number of samples since last window extraction
    int sample_counter = 0;

    TickType_t last_wake_time = xTaskGetTickCount();

    while (1) {  
        // Read a new sample from each FSR      
        for (int i = 0; i < NUM_FSRS; i++) {
            int raw_value;
            esp_err_t err = adc_oneshot_read(adc1_handle, fsr_pins[i], &raw_value);
            if (err == ESP_OK) {
                // printf("FSR%d: %d\n", i, raw_value);  // Print one FSR reading per line

                // Write new sample in circular buffer
                signal_buffer[i][buffer_index] = raw_value;
            } else {
                printf("FSR%d: ADC Read Failed (%d)\n", i, err);
            }
        }
        
        // Update buffer index and add 1 to counter
        buffer_index = (buffer_index + 1) % WINDOW_SIZE;
        sample_counter++;

        // Check if it's time to extract a new window (after 50 ms worth of data)
        if (sample_counter >= STEP_SIZE) {
            sample_counter = 0;
            
            // To store a vector of all channels at a certain time period
            Window window;
            
            // Build full window of recent samples for each FSR channel
            for (int ch = 0; ch < NUM_FSRS; ch++) {
                
                // To store a vector of one channel
                std::vector<int> channel;
                
                for (int i = 0; i < WINDOW_SIZE; i++) {
                    // Define idx as the "read index", accouting for circular buffer wrap-around
                    int idx = (buffer_index + i) % WINDOW_SIZE;
                    channel.push_back(signal_buffer[ch][idx]);
                }
                
                // Add one full channel to the window vector
                window.push_back(channel);
            }
            
            // Extract MAV features from window and send to prediction queue
            MAVFeature features = extract_mav_feature_from_window(window);
            xQueueSend(feature_queue, &features, 0);
        }
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(SAMPLE_RATE_MS));
    }
    adc_oneshot_del_unit(adc1_handle);
    vTaskDelete(NULL);
}

void predict_task(void *pvParameter) {
    while (1) {
        MAVFeature received_features;
        if (xQueueReceive(feature_queue, &received_features, portMAX_DELAY) == pdTRUE) {
            if (received_features.size() == NUM_FEATURES) {
                double input_vector[NUM_FEATURES];
                double input_scaled[NUM_FEATURES];

                for (int i = 0; i < NUM_FEATURES; i++)
                    input_vector[i] = static_cast<double>(received_features[i]);

                normalize(input_vector, input_scaled);  // From predict.h
                int prediction = predict(input_scaled); // From predict.h

                std::string label = label_map[prediction];
                ESP_LOGI(TAG, "Prediction: %d (%s)", prediction, label.c_str());
            }
        }
    }
}

extern "C" void app_main(void) {
    feature_queue = xQueueCreate(FEATURE_QUEUE_LENGTH, sizeof(MAVFeature));
    if (feature_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create feature queue.");
        return;
    }

    // FSR CODE
    xTaskCreate(&read_fsr_task, "read_fsr_task", 4096, NULL, 5, NULL);
    
   // Grasp Prediction
   xTaskCreate(&predict_task, "predict_task", 4096, NULL, 5, NULL);

    // ble_init();
    // xTaskCreate(ble_notify_task, "fmg_notify", 4096, NULL, 5, NULL);
}
