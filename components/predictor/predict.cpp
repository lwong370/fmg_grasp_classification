#include "predict.h"

void normalize(const double input_raw[NUM_FEATURES], double input_scaled[NUM_FEATURES]) {
    for (int i = 0; i < NUM_FEATURES; ++i) {
        input_scaled[i] = (input_raw[i] - FEATURE_MEANS[i]) / FEATURE_STDS[i];
    }
}

int predict(const double input[NUM_FEATURES]) {
    float best_score = -1e10;  // Very low initial value
    int best_class = -1;

    for (int i = 0; i < NUM_CLASSES; ++i) {
        float score = INTERCEPTS[i];
        for (int j = 0; j < NUM_FEATURES; ++j) {
            score += COEFFS[i][j] * input[j];
        }
        if (score > best_score) {
            best_score = score;
            best_class = i;
        }
    }
    return best_class;
}