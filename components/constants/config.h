#pragma once

#ifndef CONFIG_H
#define CONFIG_H


// C-compatible definitions
// FSR & Sampling Parameters
#define NUM_CHANNELS 1
#define NUM_FSRS 1
#define SAMPLE_RATE_MS 10           // Sample every 10ms
#define WINDOW_DURATION_MS 200
#define STEP_DURATION_MS 50
#define WINDOW_SIZE (WINDOW_DURATION_MS / SAMPLE_RATE_MS)  // 20 samples
#define STEP_SIZE (STEP_DURATION_MS / SAMPLE_RATE_MS)      // 5 samples
#define NUM_FEATURES 8
#define FEATURE_QUEUE_LENGTH 5

#ifdef __cplusplus
// C++-only includes
#include <vector>
#include <string>
// ... other C++ stuff
#endif

#endif