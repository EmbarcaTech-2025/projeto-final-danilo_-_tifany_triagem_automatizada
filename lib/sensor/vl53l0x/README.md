# VL53L0X Time-of-Flight Distance Sensor Library

This library provides an interface to the VL53L0X laser ranging sensor for accurate distance measurements on Raspberry Pi Pico.

## Features

- Accurate distance measurement up to 2 meters
- Long-range mode for extended measurements
- I2C communication
- Continuous and single-shot measurement modes
- Automatic calibration and configuration

## Usage

```c
#include "vl53l0x.h"

// Initialize sensor (channel, address, long_range_mode)
if (vl53l0x_init(1, 0x29, 0)) {
    int model, revision;
    
    // Check sensor identity
    if (vl53l0x_get_model(&model, &revision)) {
        printf("VL53L0X Model: 0x%02X, Revision: 0x%02X\n", model, revision);
    }
    
    // Read distance
    int distance = vl53l0x_read_distance();
    if (distance > 0) {
        printf("Distance: %d mm\n", distance);
    }
}
```

## API Reference

- `int vl53l0x_init(int iChan, int iAddr, int bLongRange)` - Initialize sensor
- `int vl53l0x_read_distance(void)` - Read distance in millimeters
- `uint16_t vl53l0x_read_range_continuous_mm(void)` - Continuous measurement mode
- `int vl53l0x_get_model(int *model, int *revision)` - Get sensor identification

## Configuration

- **Standard Range**: Up to ~1.2m with good accuracy
- **Long Range Mode**: Extended range up to ~2m (slightly lower accuracy)
- **I2C Address**: 0x29 (default)
- **Measurement Time**: ~20-33ms per reading

## Pin Configuration

- Uses I2C1 by default
- Configurable I2C pins through hardware setup

## Range Characteristics

- **Minimum Distance**: ~5cm
- **Maximum Distance**: 120cm (standard), 200cm (long range)
- **Accuracy**: ±3% typical
- **Resolution**: 1mm

## Notes

- Sensor requires proper power supply (3.3V)
- Works best with non-reflective, perpendicular surfaces
- Performance may vary with target material and ambient light
