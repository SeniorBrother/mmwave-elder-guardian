cmake_minimum_required(VERSION 3.16)
project(elder_guardian_host VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 需要 Qt5 组件: Widgets Charts Network SerialBus(自带 Modbus) SerialPort
find_package(Qt5 REQUIRED COMPONENTS Widgets Charts Network SerialBus SerialPort)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/modbusmaster.cpp
    src/modbusmaster.h
    src/onenetclient.cpp
    src/onenetclient.h
    src/dashboard.cpp
    src/dashboard.h
    src/otapanel.cpp
    src/otapanel.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Widgets Qt5::Charts Qt5::Network Qt5::SerialBus Qt5::SerialPort
)
