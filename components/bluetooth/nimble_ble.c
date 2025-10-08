#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "sdkconfig.h"
#include "nimble_ble.h"
#include "services/gap/ble_svc_gap.h"
#include "esp_bt.h"
#include "../components/constants/config.h"

#define NOTIFY_TASK_STACK  4096
#define NOTIFY_TASK_PRIO   5
#define FSR_QUEUE_DEPTH    1

char *TAG = "BLE-Server";
    static const char *DEVICE_NAME = "PoohBand";

uint8_t ble_addr_type;
uint16_t attr_handle;
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t sensor_data_handle;
volatile uint8_t notify_client = 0;

static QueueHandle_t s_fsr_q = NULL;   // <-- define it here

void ble_app_advertise(void);

static const ble_uuid128_t SERVICE_UUID = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
    0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef
);

static const ble_uuid128_t READ_CHAR_UUID = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
    0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0x00, 0x01
);

static const ble_uuid128_t WRITE_CHAR_UUID = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
    0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0x00, 0x02
);

static const ble_uuid128_t NOTIFY_CHAR_UUID = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef,
    0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0x00, 0x03
);

// Write data to ESP32 defined as server
static int device_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    printf("Data from the client: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);
    return 0;
}

// Read data from ESP32 defined as server
static int device_read(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    os_mbuf_append(ctxt->om, "Data from the server", strlen("Data from the server"));
    return 0;
}

static int fsr_read_cb(uint16_t ch, uint16_t ah, struct ble_gatt_access_ctxt *ctxt, void *arg){
    // put your latest cached bytes here; for now just a stub
    const char *msg = "last FSR value here";
    return os_mbuf_append(ctxt->om, msg, strlen(msg)) == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

// Array of pointers to other service definitions
// UUID - Universal Unique Identifier
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &SERVICE_UUID.u,
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = &READ_CHAR_UUID.u,
          .flags = BLE_GATT_CHR_F_READ,
          .access_cb = device_read},
         {.uuid = &WRITE_CHAR_UUID.u,
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = device_write},
         { .uuid = &NOTIFY_CHAR_UUID.u,
          .flags = BLE_GATT_CHR_F_NOTIFY | BLE_GATT_CHR_F_READ,   // (READ optional)
          .access_cb = fsr_read_cb,
          .val_handle = &sensor_data_handle },
         {0}}},
    {0}};


// BLE event handling
static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {

    case BLE_GAP_EVENT_CONNECT: // connection complete (success or fail)
        ESP_LOGI(TAG, "GAP CONNECT %s", event->connect.status == 0 ? "OK" : "FAILED");
        if (event->connect.status != 0) {
            // Connection failed, resume advertising
            ble_app_advertise();
        } else {
            // Connected
            conn_handle = event->connect.conn_handle;

            struct ble_gap_conn_desc desc;
            if (ble_gap_conn_find(conn_handle, &desc) == 0) {
                ESP_LOGI(TAG, "connected to %02X:%02X:%02X:%02X:%02X:%02X",
                         desc.peer_id_addr.val[0], 
                         desc.peer_id_addr.val[1], 
                         desc.peer_id_addr.val[2],
                         desc.peer_id_addr.val[3], 
                         desc.peer_id_addr.val[4], 
                         desc.peer_id_addr.val[5]);
            }
        }
        break;
    
    case BLE_GAP_EVENT_DISCONNECT: 
        ESP_LOGI(TAG, "GAP DISCONNECT (reason=%d)", event->disconnect.reason);
        
        // Rset conn_handle
        conn_handle   = BLE_HS_CONN_HANDLE_NONE;
        attr_handle   = 0;
        notify_client = false;
        
        // Connection terminated; rsume advertising
        ble_app_advertise();
        break;
    
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "ADV complete: reason=%d", event->adv_complete.reason);
        // Restart if not connected
        if (conn_handle == BLE_HS_CONN_HANDLE_NONE) {
            ble_app_advertise();
        }
        break;
    
    case BLE_GAP_EVENT_SUBSCRIBE: 
        ESP_LOGI(TAG, "SUBSCRIBE: conn=%d attr=%d cur_notify=%d cur_indicate=%d",
                 event->subscribe.conn_handle,
                 event->subscribe.attr_handle,
                 event->subscribe.cur_notify,
                 event->subscribe.cur_indicate);

        conn_handle   = event->subscribe.conn_handle;
        attr_handle   = event->subscribe.attr_handle;
        notify_client = event->subscribe.cur_notify || event->subscribe.cur_indicate;
        break;

    case BLE_GAP_EVENT_NOTIFY_TX: 
        struct ble_gap_conn_desc desc;
        if (ble_gap_conn_find(event->notify_tx.conn_handle, &desc) == 0) {
            ESP_LOGI(TAG,
                     "%s TX: peer=%02X:%02X:%02X:%02X:%02X:%02X attr=%u status=%d",
                     event->notify_tx.indication ? "INDIC" : "NOTIF",
                     desc.peer_id_addr.val[0], 
                     desc.peer_id_addr.val[1], 
                     desc.peer_id_addr.val[2],
                     desc.peer_id_addr.val[3], 
                     desc.peer_id_addr.val[4], 
                     desc.peer_id_addr.val[5],
                     event->notify_tx.attr_handle, event->notify_tx.status);
        } else {
            ESP_LOGW(TAG, "NOTIFY_TX: conn not found (handle=%d)", event->notify_tx.conn_handle);
        }
        break;
    
    case BLE_GAP_EVENT_MTU: 
        ESP_LOGI(TAG, "MTU updated: conn=%u cid=%u mtu=%u",
                 event->mtu.conn_handle, event->mtu.channel_id, event->mtu.value);
        break;

    case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE: 
        if (event->phy_updated.status == 0) {
            uint8_t tx_phy = 0, rx_phy = 0;
            ble_gap_read_le_phy(event->phy_updated.conn_handle, &tx_phy, &rx_phy);
            ESP_LOGI(TAG, "PHY update OK: conn=%u tx=%u rx=%u",
                     event->phy_updated.conn_handle, tx_phy, rx_phy);
        } else {
            ESP_LOGW(TAG, "PHY update failed: status=%d", event->phy_updated.status);
        }
        break;
    
    default: 
        // Generic fallback 
        ESP_LOGI(TAG, "GAP event not handled, code:%u", event->type);
        break;
    }

    return 0;
}

// Define the BLE connection
void ble_app_advertise(void) {
    struct ble_hs_adv_fields fields;
    struct ble_hs_adv_fields rsp_fields;
    struct ble_gap_adv_params adv_params = {0};

    // Fill all fields and parameters with zeros
    memset(&fields, 0, sizeof(fields));
    memset(&adv_params, 0, sizeof(adv_params));
    memset(&rsp_fields, 0, sizeof(rsp_fields));

    // Advertising data fields
    fields.name = (uint8_t *)"PoohBand";
    fields.name_len = strlen("PoohBand");
    fields.name_is_complete = 1;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE("BLE", "Failed to set advertisement data; rc=%d", rc);
        return;
    }

    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = 0x80;
    adv_params.itvl_max = 0x100;
    
    rc = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE("BLE", "Failed to start advertising; rc=%d", rc);
    }
}

bool ble_notify_ready(void) {
    return (conn_handle != BLE_HS_CONN_HANDLE_NONE) && (sensor_data_handle != 0) && (notify_client);
}

// The application
void ble_app_on_sync(void) {
    // Get a valid BLE address type
    int rc = ble_hs_id_infer_auto(0, &ble_addr_type);
    if (rc != 0) {
        ESP_LOGE("BLE", "ble_hs_id_infer_auto failed: %d", rc);
        return;
    }

    // Confirm GAP device name was set
    rc = ble_svc_gap_device_name_set("PoohBand");
    if (rc != 0) {
        ESP_LOGE("BLE", "Failed to set device name: %d", rc);
    }

    // Now that everything is synced, advertise
    ble_app_advertise();
}

// The infinite task
void host_task(void *param) {
    nimble_port_run(); // This function will return only when nimble_port_stop() is executed
}

// void app_main()
// {
//     #if !CONFIG_BT_NIMBLE_ENABLED
//         #error "NimBLE must be enabled in sdkconfig!"
//     #endif

//     nvs_flash_init();                          // 1 - Initialize NVS flash using
//     nimble_port_init();                        // 3 - Initialize the host stack
//     ble_svc_gap_device_name_set("BLE-Server"); // 4 - Initialize NimBLE configuration - server name
//     ble_svc_gap_init();                        // 4 - Initialize NimBLE configuration - gap service
//     ble_svc_gatt_init();                       // 4 - Initialize NimBLE configuration - gatt service
//     ble_gatts_count_cfg(gatt_svcs);            // 4 - Initialize NimBLE configuration - config gatt services
//     ble_gatts_add_svcs(gatt_svcs);             // 4 - Initialize NimBLE configuration - queues gatt services.
//     ble_hs_cfg.sync_cb = ble_app_on_sync;      // 5 - Initialize application
//     nimble_port_freertos_init(host_task);      // 6 - Run the thread
// }

// Pick a max elements constant used consistently across BLE code
#ifndef BLE_FSR_MAX_ELEMS
#define BLE_FSR_MAX_ELEMS  16   // or NUM_FSRS, but keep it >= max n you’ll send
#endif

typedef struct {
    size_t  len;                                          // payload length in bytes
    uint8_t bytes[BLE_FSR_MAX_ELEMS * sizeof(int)];       // storage
} fsr_payload_t;


bool ble_send_fsr_sample(const int *data, size_t n)
{

    if (!s_fsr_q || !data || n == 0 || n > BLE_FSR_MAX_ELEMS) {
        return false;
    }

    // fsr_payload_t p;
    // p.len = n * sizeof(int);
    // memcpy(p.bytes, data, p.len);

    // BaseType_t sent = xQueueSend(s_fsr_q, &p, 0);
    // return sent == pdTRUE;

    fsr_payload_t p;
    p.len = n * sizeof(int32_t);
    memcpy(p.bytes, data, p.len);
    return xQueueOverwrite(s_fsr_q, &p) == pdTRUE;
}


void ble_notify_task(void *param) {
    //float fsr_data[NUM_FSRS] = {2};
    fsr_payload_t p;

    while(1)
    {
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE && sensor_data_handle != 0 && notify_client)
        {
            
           if (xQueueReceive(s_fsr_q, &p, portMAX_DELAY) != pdTRUE) continue;

            // Create mbuf to hold data
            struct os_mbuf *om = ble_hs_mbuf_from_flat(p.bytes, p.len);
            
            if (om != NULL)
            {
                int rc = ble_gatts_notify_custom(conn_handle, sensor_data_handle, om);
                if (rc == 0)
                {
                    //ESP_LOGI(TAG, "Notification sent: [%.1f", fsr_data[0]);
                    ESP_LOGI(TAG, "Notification sent: [%.1f", 2.0);
                    int first = 0;
                    if (p.len >= sizeof(int)) memcpy(&first, p.bytes, sizeof(int));
                        ESP_LOGI(TAG, "Notify queued: attr=0x%04x bytes=%u first=%d", sensor_data_handle, (unsigned)p.len, first);
                    }
                else
                {
                    ESP_LOGE(TAG, "Error sending notification: %d", rc);
                }
            }
            else
            {
                ESP_LOGE(TAG, "Failed to allocate mbuf");
            }
        }
        else
        {
            ESP_LOGW(TAG, "No active connection, waiting...");
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));  // Send every 500ms (adjust as needed)
    }
}

// void ble_notify_task(void *param)
// {
//     fsr_payload_t p;
//     ESP_LOGI(TAG, "notify task started; q=%p", (void*)s_fsr_q);

//     for (;;) {
//         if (conn_handle == BLE_HS_CONN_HANDLE_NONE || sensor_data_handle == 0 || !notify_client) {
//             ESP_LOGD(TAG, "not ready: conn=%d handle=0x%04x sub=%d qwait=%u",
//                      conn_handle, sensor_data_handle, notify_client,
//                      (unsigned)uxQueueMessagesWaiting(s_fsr_q));
//             vTaskDelay(pdMS_TO_TICKS(100));
//             continue;
//         }

//         //TODO: Issue here bc nothing added to queue
//         if (xQueueReceive(s_fsr_q, &p, pdMS_TO_TICKS(1000)) != pdTRUE) {
//             ESP_LOGW(TAG, "no packet in 1s; qwait=%u", (unsigned)uxQueueMessagesWaiting(s_fsr_q));
//             continue;
//         }

//         struct os_mbuf *om = ble_hs_mbuf_from_flat(p.bytes, p.len);
//         if (!om) { ESP_LOGE(TAG, "mbuf alloc failed"); continue; }

//         int rc = ble_gatts_notify_custom(conn_handle, sensor_data_handle, om);
//         if (rc == 0) {
//             int first = 0; if (p.len >= (int)sizeof(int)) memcpy(&first, p.bytes, sizeof(int));
//             ESP_LOGI(TAG, "Notify queued: attr=0x%04x bytes=%u first=%d",
//                      sensor_data_handle, (unsigned)p.len, first);
//         } else {
//             ESP_LOGE(TAG, "Notify failed rc=%d", rc);
//             // os_mbuf_free_chain(om); // only if stack didn't consume on error
//         }
//     }
// }

void ble_init() {
    esp_err_t ret;
    esp_log_level_set(TAG, ESP_LOG_DEBUG);

    if (!s_fsr_q) s_fsr_q = xQueueCreate(FSR_QUEUE_DEPTH, sizeof(fsr_payload_t));
    ESP_LOGI(TAG, "FSR queue created: %p (item=%u bytes)", (void*)s_fsr_q, (unsigned)sizeof(fsr_payload_t));
    xTaskCreate(ble_notify_task, "ble_notify_task", 4096, NULL, 6, NULL); // prio > sensor

    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // // Release Classic BT memory
    // ret = esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "Failed to release Classic BT memory: %s", esp_err_to_name(ret));
    //     return;
    // }

    // // Check controller status
    // esp_bt_controller_status_t status = esp_bt_controller_get_status();
    // ESP_LOGI(TAG, "BT controller status before init: %d", status);
    // if (status == ESP_BT_CONTROLLER_STATUS_IDLE) {
    //     // Initialize controller
    //     esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    //     ret = esp_bt_controller_init(&bt_cfg);
    //     if (ret != ESP_OK) {
    //         ESP_LOGE(TAG, "Bluetooth controller initialization failed: %s", esp_err_to_name(ret));
    //         return;
    //     }
    // } else {
    //     ESP_LOGW(TAG, "Bluetooth controller already initialized or in unexpected state: %d", status);
    // }

    // // Enable controller
    // ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
    //     return;
    // }
    
    //ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());

    // Initialize NimBLE
    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NimBLE port initialization failed: %s", esp_err_to_name(ret));
        return;
    }

    // Initialize GAP and GATT
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // Set device name
    int rc = ble_svc_gap_device_name_set(DEVICE_NAME);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set device name: %d", rc);
    }

    // Configure GATT services
    rc = ble_gatts_count_cfg(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to count GATT services: %d", rc);
    }

    rc = ble_gatts_add_svcs(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to add GATT services: %d", rc);
    }

    // Set sync callback
    ble_hs_cfg.sync_cb = ble_app_on_sync;

    // Start host task
    nimble_port_freertos_init(host_task);

    ESP_LOGI(TAG, "BLE initialization complete");
}