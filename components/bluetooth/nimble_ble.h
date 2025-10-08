#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void ble_init(void);

void ble_notify_task(void *param);

bool ble_notify_ready(void);

bool ble_send_notification(const uint8_t *data, size_t len);

bool ble_send_fsr_sample(const int *data, size_t n);

#ifdef __cplusplus
}
#endif
