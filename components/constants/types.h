#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>
#include "config.h"

typedef struct {
    uint8_t addr;
    int data;
} sensor_x;

typedef struct {
    int index;
    sensor_x ch[MAX_SENSOR_CHANNELS];
} sensor_sample_t;

#endif