# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(EXTRA_COMPONENT_DIRS
    ${CMAKE_SOURCE_DIR}/components/constants
    ${CMAKE_SOURCE_DIR}/components/feature_extraction
    ${CMAKE_SOURCE_DIR}/components/bluetooth
    ${CMAKE_SOURCE_DIR}/components/predictor
)
project(GPIO_Example)
# set(EXTRA_COMPONENT_DIRS "$/Users/lanaw/BEAR_lab/pooh_band/learning_for_dev/esp/esp_liblinear/liblinear")
