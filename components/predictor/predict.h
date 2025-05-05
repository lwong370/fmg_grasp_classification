#pragma once
#include "lda_model.h"

void normalize(const double input_raw[NUM_FEATURES], double input_scaled[NUM_FEATURES]);
int predict(const double input[NUM_FEATURES]);