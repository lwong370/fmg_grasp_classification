idf_component_register(
    SRCS "nimble_ble.c"
    INCLUDE_DIRS "."
    PRIV_INCLUDE_DIRS 
        "C:/Users/lanaw/esp/frameworks/esp-idf-v5.3.1/components/bt/host/nimble/esp-hci/include"
        "C:/Users/lanaw/esp/frameworks/esp-idf-v5.3.1/components/bt/host/nimble/nimble/nimble/transport/include"
        "C:/Users/lanaw/esp/frameworks/esp-idf-v5.3.1/components/bt/host/nimble/nimble/porting/nimble/include"
        "C:/Users/lanaw/esp/frameworks/esp-idf-v5.3.1/components/bt/host/nimble/port/include"
        "C:/Users/lanaw/esp/frameworks/esp-idf-v5.3.1/components/bt/porting/npl/freertos/include"
        "C:/Users/lanaw/esp/frameworks/esp-idf-v5.3.1/components/bt/host/nimble/nimble/nimble/host/include"
        "C:/Users/lanaw/esp/frameworks/esp-idf-v5.3.1/components/bt/host/nimble/nimble/nimble/include"
        "C:/Users/lanaw/esp/frameworks/esp-idf-v5.3.1/components/bt/host/nimble/nimble/nimble/host/services/gap/include"
        "C:/Users/lanaw/esp/frameworks/esp-idf-v5.3.1/components/bt/host/nimble/nimble/nimble/host/services/gatt/include"
    REQUIRES nvs_flash bt esp_timer driver esp_hw_support esp_event
)
