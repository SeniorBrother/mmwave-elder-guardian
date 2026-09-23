# ESP32-S3 目标顶层 CMake（ESP-IDF v5.x）
# 用法:
#   . $IDF_PATH/export.sh
#   idf.py set-target esp32s3
#   idf.py build flash monitor

cmake_minimum_required(VERSION 3.16)

# 引入跨平台业务层 common 作为一个组件
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_LIST_DIR}/../common"
    "${CMAKE_CURRENT_LIST_DIR}/components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mmwave_elder_guardian)
