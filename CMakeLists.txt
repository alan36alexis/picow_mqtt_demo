# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(pico_sdk_import.cmake)

project(pico_mqtt_test C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Hacer que lwipopts.h sea visible globalmente para asegurar que las librerías
# del SDK lo encuentren durante su compilación.
include_directories(${CMAKE_CURRENT_LIST_DIR})

pico_sdk_init()

add_executable(pico_mqtt_test main.c)

# Habilitar salida USB para ver logs en el monitor serial
pico_enable_stdio_usb(pico_mqtt_test 0)
pico_enable_stdio_uart(pico_mqtt_test 1)

target_link_libraries(pico_mqtt_test
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip
    pico_lwip_mqtt
    pico_lwip_arch
)

pico_add_extra_outputs(pico_mqtt_test)