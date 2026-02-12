# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()

set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(USB_DEVICE_AND_KEYBOARD)

pico_sdk_init()

add_subdirectory("./Pico-PIO-USB" pico_pio_usb)

set(target_name USB_DEVICE_AND_KEYBOARD)

# Generate FAT12 disk image from storage_files directory
# Find Python (handles both 'python' on Windows and 'python3' on Unix)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/storage_data.c
    COMMAND ${CMAKE_COMMAND} -E echo "Generating FAT12 disk image..."
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_fat12_image.py
            ${CMAKE_CURRENT_SOURCE_DIR}/storage_files
            ${CMAKE_CURRENT_BINARY_DIR}/storage_data.c
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_fat12_image.py
    COMMENT "Generating FAT12 disk image from storage_files/"
    VERBATIM
)

add_executable(${target_name})
target_sources(${target_name} PRIVATE
    main_device.c
    main_host.c
    usb_descriptors.c
    uart_comm.c
    msc_storage.c
    net_lwip.c
    lib/networking/dhserver.c
    lib/networking/dnserver.c
    lib/networking/rndis_reports.c
    ${CMAKE_CURRENT_BINARY_DIR}/storage_data.c
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)


target_link_options(${target_name} PRIVATE -Xlinker --print-memory-usage)
target_compile_definitions(${target_name} PRIVATE PIO_USB_USE_TINYUSB)
target_include_directories(${target_name} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/arch
    ${CMAKE_CURRENT_LIST_DIR}/lib
    ${CMAKE_CURRENT_LIST_DIR}/lib/networking
)
target_link_libraries(${target_name} PRIVATE pico_stdlib pico_pio_usb tinyusb_device tinyusb_host hardware_uart pico_lwip)
pico_add_extra_outputs(${target_name})