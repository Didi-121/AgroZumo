cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(Pico2wCAN C CXX ASM)

pico_sdk_init()

add_executable(Pico2wCAN
    main.c
    mcp2515.c
)

pico_set_program_name(Pico2wCAN "Pico2wCAN")
pico_set_program_version(Pico2wCAN "0.1")

pico_enable_stdio_uart(Pico2wCAN 0)
pico_enable_stdio_usb(Pico2wCAN 1)

target_link_libraries(Pico2wCAN
    pico_stdlib
    hardware_spi
    pico_cyw43_arch_lwip_poll
)

target_include_directories(Pico2wCAN PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(Pico2wCAN)