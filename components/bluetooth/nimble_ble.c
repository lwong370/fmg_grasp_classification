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

char *TAG = "BLE-Server";
static const char *DEVICE_NAME = "PoohBand";

uint8_t ble_addr_type;
void ble_app_advertise(void);

// Write data to ESP32 defined as server
static int device_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    printf("Data from the client: %.*s\n", ctxt->om->om_len, ctxt->om->om_data);
    return 0;
}

// Read data from ESP32 defined as server
static int device_read(uint16_t con_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    os_mbuf_append(ctxt->om, "Data from the server", strlen("Data from the server"));
    return 0;
}

// Array of pointers to other service definitions
// UUID - Universal Unique Identifier
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = BLE_UUID16_DECLARE(0x180),                 // Define UUID for device type
     .characteristics = (struct ble_gatt_chr_def[]){
         {.uuid = BLE_UUID16_DECLARE(0xFEF4),           // Define UUID for reading
          .flags = BLE_GATT_CHR_F_READ,
          .access_cb = device_read},
         {.uuid = BLE_UUID16_DECLARE(0xDEAD),           // Define UUID for writing
          .flags = BLE_GATT_CHR_F_WRITE,
          .access_cb = device_write},
         {0}}},
    {0}};

// BLE event handling
static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type)
    {
    // Advertise if connected
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI("GAP", "BLE GAP EVENT CONNECT %s", event->connect.status == 0 ? "OK!" : "FAILED!");
        if (event->connect.status != 0)
        {
            ble_app_advertise();
        }
        break;
    // Advertise again after completion of the event
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI("GAP", "BLE GAP EVENT");
        ble_app_advertise();
        break;
    default:
        break;
    }
    return 0;
}

// Define the BLE connection
void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));
    fields.name = (uint8_t *)"PoohBand";
    fields.name_len = strlen("PoohBand");
    fields.name_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE("BLE", "Failed to set advertisement data; rc=%d", rc);
        return;
    }

    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = 0x80;
    adv_params.itvl_max = 0x100;

    rc = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE("BLE", "Failed to start advertising; rc=%d", rc);
    }
}

// The application
void ble_app_on_sync(void)
{
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
void host_task(void *param)
{
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

void ble_init()
{
    esp_err_t ret;

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
    // ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());

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
