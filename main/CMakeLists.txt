idf_component_register(SRCS "GPIO_Example.cpp" "../liblinear/linear.cpp"
                       INCLUDE_DIRS "." "../liblinear")

set(CMAKE_CXX_STANDARD 17)
target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu++17)
spiffs_create_partition_image(storage ../partition FLASH_IN_PROJECT)

# idf_component_register(
#     SRCS "bluetooth_ble.c" "GPIO_Example.cpp" "../liblinear/linear.cpp"
#     INCLUDE_DIRS "." "../liblinear"
#     REQUIRES bt nvs_flash esp_driver_gpio spiffs driver esp_adc 
# )

# set(CMAKE_CXX_STANDARD 17)
# set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/components/bt)

# # Only apply C++ standard to C++ files
# target_compile_options(${COMPONENT_LIB} PRIVATE 
#     $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>
# )

# spiffs_create_partition_image(storage ../partition FLASH_IN_PROJECT)
