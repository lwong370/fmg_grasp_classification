#include <vector>
#include <cmath>
#include <array>
#include "../constants/config.h"
#include "feature_extraction.h"


int contraction_time = 3;                          // 3 seconds
double center_point = 0.7;                         // 70% of data we want to look at
double time_band = contraction_time * center_point;  // how much 
double half_time_band = time_band/2.0;

// const int NUM_CHANNELS = 8;
// Create type alias for code readability
using MAVFeatureSet = std::vector<MAVFeature>; // One feature vector per window

// Calculate Mean Abso;ute Value for one Channel
float compute_mav(const std::vector<int>& signal) {
    float sum = 0.0f;
    for (int value : signal) {
        sum += std::abs(value);
    }
    return sum / signal.size();
}

// Calculate MAV for one window
MAVFeature extract_mav_feature_from_window(const Window& window) {
    MAVFeature features;
    for (int channel = 0; channel < NUM_CHANNELS; channel++) {
        features[channel] = compute_mav(window[channel]); 
    }
    return features;
}

