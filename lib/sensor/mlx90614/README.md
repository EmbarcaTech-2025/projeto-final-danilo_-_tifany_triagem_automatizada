# MLX90614 Temperature Sensor Library

This library provides a clean interface to the MLX90614 non-contact infrared temperature sensor for Raspberry Pi Pico.

## Features

- Read ambient temperature
- Read object (body) temperature 
- I2C communication with configurable address
- Temperature readings in Celsius

## Usage

```c
#include "mlx90614.h"
#include "hardware/i2c.h"

// Initialize I2C
i2c_init(i2c1, 400000);
gpio_set_function(2, GPIO_FUNC_I2C); // SDA
gpio_set_function(3, GPIO_FUNC_I2C); // SCL
gpio_pull_up(2);
gpio_pull_up(3);

// Initialize sensor
if (mlx90614_init(i2c1, MLX90614_DEFAULT_ADDR)) {
    float ambient_temp, object_temp;
    
    // Read temperatures
    if (mlx90614_read_ambient_c(&ambient_temp)) {
        printf("Ambient: %.2f°C\n", ambient_temp);
    }
    
    if (mlx90614_read_object_c(&object_temp)) {
        printf("Object: %.2f°C\n", object_temp);
    }
}
```

## API Reference

- `int mlx90614_init(i2c_inst_t *i2c, uint8_t addr)` - Initialize the sensor
- `bool mlx90614_read_ambient_c(float *temp_c)` - Read ambient temperature
- `bool mlx90614_read_object_c(float *temp_c)` - Read object temperature

## Configuration

- Default I2C address: 0x5A
- Supports custom I2C addresses
- Temperature range: -70°C to +380°C
