#ifndef FEATURE_EXTRACTION_H
#define FEATURE_EXTRACTION_H

#include <vector>
#include "../constants/types.h"

float compute_mav(const std::vector<int>& signal);
MAVFeature extract_mav_feature_from_window(const Window& window);

#endif