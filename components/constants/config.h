#pragma once

#ifndef CONFIG_H
#define CONFIG_H


// C-compatible definitions
// FSR & Sampling Parameters
extern int num_sensor_chls;

#define MAX_SENSOR_CHANNELS 12
#define SAMPLE_RATE_MS 10   // Sample every 10ms
#define WINDOW_DURATION_MS 200
#define STEP_DURATION_MS 50

extern const float VREF; 

#ifdef __cplusplus
// C++-only includes
#include <vector>
#include <string>
// ... other C++ stuff
#endif

#endif