#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void ble_init(void);

void ble_notify_task(void *param);

#ifdef __cplusplus
}
#endif
