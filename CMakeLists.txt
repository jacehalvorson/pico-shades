cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

include(pico_extras_import_optional.cmake)

project(shades C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PROJR ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()

include(example_auto_set_url.cmake)

add_compile_options(-Wall
    -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
    -Wno-unused-function # we have some for the docs that aren't called
)
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wno-maybe-uninitialized)
endif()

if (NOT PICO_CYW43_SUPPORTED AND NOT TARGET pico_cyw43_arch) # set by PICO_BOARD=pico_w
    message("WARNING: Pico W support is not available")
else()
    if (DEFINED ENV{WIFI_SSID} AND (NOT WIFI_SSID))
        set(WIFI_SSID $ENV{WIFI_SSID})
        message("Using WIFI_SSID from environment ('${WIFI_SSID}')")
    endif()

    if (DEFINED ENV{WIFI_PASSWORD} AND (NOT WIFI_PASSWORD))
        set(WIFI_PASSWORD $ENV{WIFI_PASSWORD})
        message("Using WIFI_PASSWORD from environment")
    endif()

    set(WIFI_SSID "${WIFI_SSID}" CACHE INTERNAL "WiFi SSID for examples")
    set(WIFI_PASSWORD "${WIFI_PASSWORD}" CACHE INTERNAL "WiFi password for examples")
    message("Wifi target is '${WIFI_SSID}' with password '${WIFI_PASSWORD}'")
endif()

add_executable(shades
    http.c
    set_alarm.c
    set_rtc_time.c
    shades.c
    tcp_server.c
    utils.c
)

target_include_directories(shades PRIVATE ${CMAKE_CURRENT_LIST_DIR} )

# pull in common dependencies
target_link_libraries(shades pico_stdlib hardware_rtc pico_cyw43_arch_lwip_threadsafe_background)

target_compile_definitions(shades PRIVATE
    WIFI_SSID=\"${WIFI_SSID}\"
    WIFI_PASSWORD=\"${WIFI_PASSWORD}\"
)

pico_enable_stdio_uart(shades 0)

if (TARGET tinyusb_device)
    message("Enabling USB output")
    pico_enable_stdio_usb(shades 1)
else()
    message("WARNING: Compiling without debug messages over USB")
    pico_enable_stdio_usb(shades 0)
endif()

# create map/bin/hex file etc.
pico_add_extra_outputs(shades)
