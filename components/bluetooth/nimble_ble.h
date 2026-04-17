#ifndef NIMBLE_BLE_H
#define NIMBLE_BLE_H

#include <stddef.h>
#include "../constants/types.h"

#ifdef __cplusplus
extern "C" {
#endif

void ble_init(void);

void ble_notify_task(void *param);

bool ble_notify_ready(void);

bool ble_send_notification(const uint8_t *data, size_t len);

void ble_send_fsr_sample(const sensor_x *udata, size_t n);

#ifdef __cplusplus
}
#endif

#endif
