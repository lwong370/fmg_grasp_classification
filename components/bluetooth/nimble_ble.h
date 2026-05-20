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


/**
 * @brief FreeRTOS task that sends BLE notifications to connected client.
 *
 * Waits for FSR sample payloads on bt_input_queue and forwards them as
 * GATT notifications over an active BLE connection. Runs continuously;
 * skips notification if no client is connected or notifications are disabled.
 *
 * @param param  Unused task parameter (required by FreeRTOS task signature)
 */
void ble_notify_task(void *param);


/**
 * @brief Check if BLE is ready to send notifications.
 *
 * @return true if a client is connected and notifications are enabled,
 *         false if not
 */
bool ble_notify_ready(void);


/**
 * @brief Enqueue FSR sample for BLE notification.
 *
 * Packs n sensor readings into a payload and adds it to bt_input_queue
 * for transmission by ble_notify_task. Drops the sample if the queue is full.
 *
 * @param data  Pointer to array of sensor readings of type sensor_x
 * @param n     Number of channels in data
 */
void ble_send_fsr_sample(const sensor_x *udata, size_t n);

#ifdef __cplusplus
}
#endif

#endif
