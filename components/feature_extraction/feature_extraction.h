#ifndef FEATURE_EXTRACTION_H
#define FEATURE_EXTRACTION_H

#include <vector>

constexpr int NUM_FSRS = 8;
constexpr int WINDOW_SIZE = 400;

using Window = std::vector<std::vector<int>>;  // 8x400 per window
using MAVFeature = std::vector<float>;         // 1 MAV per channel

float compute_mav(const std::vector<int>& signal);
MAVFeature extract_mav_feature_from_window(const Window& window);

#endif