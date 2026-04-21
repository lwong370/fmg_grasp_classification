#ifndef NIMBLE_BLE_H
#define NIMBLE_BLE_H

#include <stddef.h>
#include "../constants/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the NimBLE Bluetooth Low Energy stack.
 *
 * Sets up the FSR data queue, configures GAP/GATT services, sets the
 * device name and preferred MTU, then starts the NimBLE host task.
 * Must be called once at startup before any BLE operations.
 */
void ble_init(void);

void ble_notify_task(void *param);

bool ble_notify_ready(void);

bool ble_send_notification(const uint8_t *data, size_t len);

void ble_send_fsr_sample(const sensor_x *udata, size_t n);

#ifdef __cplusplus
}
#endif

#endif
