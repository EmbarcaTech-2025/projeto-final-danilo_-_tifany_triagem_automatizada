#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifndef I2C_PORT
#define I2C_PORT i2c1
#endif
#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN 2
#endif
#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN 3
#endif

#define MAX3010X_I2C_ADDR 0x57

// Forward declaration to avoid requiring Pico SDK headers in this public header
typedef struct i2c_inst i2c_inst_t;

typedef struct {
    i2c_inst_t *i2c;
    uint8_t address;
    uint8_t active_leds; // 1=red, 2=red+ir
} max3010x_t;

typedef struct {
    uint32_t red;
    uint32_t ir;
} max3010x_sample_t;

// Init I2C pins, verify part id, and reset the device.
bool max3010x_init(max3010x_t *dev);

// Configure for SpO2 collection (Red + IR) with given parameters.
bool max3010x_configure_spo2(max3010x_t *dev,
                             uint8_t led_brightness,  // 0..255
                             uint8_t sample_average,  // 1,2,4,8,16,32
                             int sample_rate,         // 50..3200
                             int pulse_width,         // 69,118,215,411
                             int adc_range);          // 2048,4096,8192,16384

// FIFO helpers
bool max3010x_clear_fifo(max3010x_t *dev);
bool max3010x_read_sample(max3010x_t *dev, max3010x_sample_t *out); // blocks until available

// Low-level register helpers
bool max3010x_write_reg(max3010x_t *dev, uint8_t reg, uint8_t val);
bool max3010x_read_reg(max3010x_t *dev, uint8_t reg, uint8_t *val);
bool max3010x_bitmask(max3010x_t *dev, uint8_t reg, uint8_t mask, uint8_t thing);
