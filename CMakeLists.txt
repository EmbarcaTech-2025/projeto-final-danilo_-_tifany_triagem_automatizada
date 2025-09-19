# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

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
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Set any variables required for importing libraries
if (DEFINED ENV{FREERTOS_KERNEL_PATH})
  SET(FREERTOS_KERNEL_PATH $ENV{FREERTOS_KERNEL_PATH})
else()
  SET(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_LIST_DIR}/FreeRTOS-Kernel)
endif()

message("FreeRTOS Kernel located in ${FREERTOS_KERNEL_PATH}")


# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(triagem_completo C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(triagem_completo
        src/main.c
        src/tasks/task_fsm.c
        src/tasks/task_ui.c
        src/tasks/task_audio.c
        src/tasks/task_ppg.c
        src/tasks/task_temp_dist.c
        src/tasks/task_server.c
        src/tasks/task_wifi_check.c
        src/tasks/task_hooks.c
        src/drivers/wifi_client.c
        src/drivers/nextion.c
        src/drivers/audio.c
        src/utils/json_logger.c
        include/hw_config.c)

pico_set_program_name(triagem_completo "triagem_completo")
pico_set_program_version(triagem_completo "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(triagem_completo 0)
pico_enable_stdio_usb(triagem_completo 1)

# Add organized sensor libraries
add_subdirectory(lib/sensor/mlx90614)
add_subdirectory(lib/sensor/max3010x) 
add_subdirectory(lib/sensor/vl53l0x)

# Add the subdirectory containing the SD card library
add_subdirectory(lib/pico_sdcard)

# Add the subdirectory containing the PicoUtil Buffer library
add_subdirectory(lib/pico_util_buffer)

# Add the subdirectory containing the Pico Audio library
add_subdirectory(lib/pico_audio)

# Add the subdirectory containing the Pico Audio I2S library
add_subdirectory(lib/pico_audio_i2s)

target_include_directories(triagem_completo PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/drivers
    ${CMAKE_CURRENT_LIST_DIR}/src/utils

    )

    target_include_directories(triagem_completo PRIVATE
    ${PICO_SDK_PATH}/src/rp2_common/pico_lwip/include
)

# Add the standard library to the build
target_link_libraries(triagem_completo
        pico_stdlib
        hardware_i2c
        pico_util_buffer
        pico_audio
        pico_audio_i2s
        pico_sdcard
        mlx90614
        max3010x
        vl53l0x
        FreeRTOS-Kernel-Heap4
        pico_cyw43_arch_lwip_sys_freertos
        )

# 🔹 CONFIGURAÇÕES FREERTOS + LWIP
target_compile_definitions(triagem_completo PRIVATE
    PICO_CYW43_ARCH_FREERTOS=1
)

# Add the standard include files to the build
target_include_directories(triagem_completo PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(triagem_completo 
        
        )

pico_add_extra_outputs(triagem_completo)

