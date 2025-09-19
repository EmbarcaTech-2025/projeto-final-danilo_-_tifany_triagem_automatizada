# MAX3010X Heart Rate and SpO2 Sensor Library

This library provides a complete interface to the MAX30102/MAX30105 heart rate and blood oxygen (SpO2) sensors for Raspberry Pi Pico.

## Features

- Heart rate monitoring with beat detection
- SpO2 (blood oxygen saturation) measurement
- Configurable LED brightness and sample rates
- Real-time signal processing algorithms
- FIFO buffer management

## Modules

- **max3010x.h/c** - Low-level sensor communication and configuration
- **heart_rate.h/c** - Heart beat detection algorithm
- **spo2_algorithm.h/c** - SpO2 calculation using Maxim's algorithm

## Usage

```c
#include "max3010x.h"
#include "heart_rate.h"
#include "spo2_algorithm.h"

// Initialize sensor
max3010x_t sensor;
if (max3010x_init(&sensor)) {
    // Configure for SpO2 measurement
    max3010x_configure_spo2(&sensor, 60, 4, 100, 411, 4096);
    
    uint32_t ir_buffer[100], red_buffer[100];
    int32_t spo2, heart_rate;
    int8_t spo2_valid, hr_valid;
    
    // Collect samples
    for (int i = 0; i < 100; i++) {
        max3010x_sample_t sample;
        if (max3010x_read_sample(&sensor, &sample)) {
            ir_buffer[i] = sample.ir;
            red_buffer[i] = sample.red;
            
            // Check for heartbeat
            if (checkForBeat(sample.ir)) {
                printf("Beat detected!\n");
            }
        }
    }
    
    // Calculate SpO2 and heart rate
    maxim_heart_rate_and_oxygen_saturation(
        ir_buffer, 100, red_buffer, 
        &spo2, &spo2_valid, &heart_rate, &hr_valid
    );
    
    if (hr_valid) printf("Heart Rate: %ld BPM\n", heart_rate);
    if (spo2_valid) printf("SpO2: %ld%%\n", spo2);
}
```

## Configuration Options

- **LED Brightness**: 0-255 (affects measurement range)
- **Sample Rate**: 50-3200 Hz
- **Pulse Width**: 69-411 µs (affects resolution)
- **ADC Range**: 2048-16384 nA (affects sensitivity)

## Pin Configuration

- I2C SDA: GPIO 2 (default)
- I2C SCL: GPIO 3 (default)  
- I2C Address: 0x57

## Algorithms

- **Beat Detection**: Real-time FIR filtering and peak detection
- **SpO2 Calculation**: Maxim's proprietary algorithm with lookup table
- **Heart Rate**: Peak interval analysis with moving average
