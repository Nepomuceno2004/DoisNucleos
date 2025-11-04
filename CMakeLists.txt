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

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)
project(DoisNucleos C CXX ASM)
pico_sdk_init()

add_executable(DoisNucleos 
        DoisNucleos.c 
        lib/ssd1306/ssd1306.c
        lib/aht20/aht20.c
        lib/bmp280/bmp280.c
)

pico_set_program_name(DoisNucleos "DoisNucleos")
pico_set_program_version(DoisNucleos "0.1")

pico_enable_stdio_uart(DoisNucleos 0)
pico_enable_stdio_usb(DoisNucleos 1)

target_link_libraries(${PROJECT_NAME} 
        pico_stdlib 
        pico_multicore
        hardware_pio
        hardware_i2c
        hardware_adc
        )

target_include_directories(DoisNucleos PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/lib/aht20
    ${CMAKE_CURRENT_LIST_DIR}/lib/ssd1306
    ${CMAKE_CURRENT_LIST_DIR}/lib/bmp280
)

pico_add_extra_outputs(DoisNucleos)

