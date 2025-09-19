cmake_minimum_required(VERSION 3.13)

add_library(max3010x 
    max3010x.c
    heart_rate.c
    spo2_algorithm.c
    ppg_blood_pressure.c
)

target_include_directories(max3010x PUBLIC .)

target_link_libraries(max3010x PUBLIC 
    pico_stdlib 
    hardware_i2c
)
