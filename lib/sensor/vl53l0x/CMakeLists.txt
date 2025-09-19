cmake_minimum_required(VERSION 3.13)

add_library(vl53l0x vl53l0x.c)

target_include_directories(vl53l0x PUBLIC .)

target_link_libraries(vl53l0x PUBLIC 
    pico_stdlib 
    hardware_i2c
)
