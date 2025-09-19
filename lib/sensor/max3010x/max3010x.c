#include "max3010x.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Registers (subset)
enum {
    REG_INTSTAT1         = 0x00,
    REG_INTSTAT2         = 0x01,
    REG_INTENABLE1       = 0x02,
    REG_INTENABLE2       = 0x03,
    REG_FIFOWRITEPTR     = 0x04,
    REG_FIFOOVERFLOW     = 0x05,
    REG_FIFOREADPTR      = 0x06,
    REG_FIFODATA         = 0x07,
    REG_FIFOCONFIG       = 0x08,
    REG_MODECONFIG       = 0x09,
    REG_PARTICLECONFIG   = 0x0A, // SPO2 config
    REG_LED1_PULSEAMP    = 0x0C, // Red
    REG_LED2_PULSEAMP    = 0x0D, // IR
    REG_MULTILEDCONFIG1  = 0x11,
    REG_MULTILEDCONFIG2  = 0x12,
    REG_DIETEMPCONFIG    = 0x21,
    REG_PROXINTTHRESH    = 0x30,
    REG_REVISIONID       = 0xFE,
    REG_PARTID           = 0xFF, // 0x15 for MAX30102
};

// Bit fields and masks
#define MODE_MASK        0xF8
#define MODE_REDONLY     0x02
#define MODE_REDIRONLY   0x03

#define ADCRANGE_MASK    0x9F
#define ADCRANGE_2048    0x00
#define ADCRANGE_4096    0x20
#define ADCRANGE_8192    0x40
#define ADCRANGE_16384   0x60

#define SAMPLERATE_MASK  0xE3
#define SR_50            0x00
#define SR_100           0x04
#define SR_200           0x08
#define SR_400           0x0C
#define SR_800           0x10
#define SR_1000          0x14
#define SR_1600          0x18
#define SR_3200          0x1C

#define PWIDTH_MASK      0xFC
#define PW_69            0x00
#define PW_118           0x01
#define PW_215           0x02
#define PW_411           0x03

#define SAMPLEAVG_MASK   0xE0
#define AVG_1            0x00
#define AVG_2            0x20
#define AVG_4            0x40
#define AVG_8            0x60
#define AVG_16           0x80
#define AVG_32           0xA0

#define ROLLOVER_MASK    0xEF
#define ROLLOVER_ENABLE  0x10
#define A_FULL_MASK      0xF0

#define SHUTDOWN_MASK    0x7F
#define SHUTDOWN         0x80
#define WAKEUP           0x00

#define RESET_MASK       0xBF
#define RESET_BIT        0x40

// Slots for multi-LED (we use 1=red, 2=ir)
#define SLOT1_MASK       0xF8
#define SLOT2_MASK       0x8F
#define SLOT_RED_LED     0x01
#define SLOT_IR_LED      0x02

static uint8_t map_adc_range(int adc) {
    if (adc < 4096) return ADCRANGE_2048;
    if (adc < 8192) return ADCRANGE_4096;
    if (adc < 16384) return ADCRANGE_8192;
    return ADCRANGE_16384;
}
static uint8_t map_sample_rate(int sr) {
    if (sr < 100) return SR_50;
    if (sr < 200) return SR_100;
    if (sr < 400) return SR_200;
    if (sr < 800) return SR_400;
    if (sr < 1000) return SR_800;
    if (sr < 1600) return SR_1000;
    if (sr < 3200) return SR_1600;
    return SR_3200;
}
static uint8_t map_pwidth(int pw) {
    if (pw < 118) return PW_69;
    if (pw < 215) return PW_118;
    if (pw < 411) return PW_215;
    return PW_411;
}
static uint8_t map_avg(uint8_t avg) {
    switch (avg) {
        case 1: return AVG_1; case 2: return AVG_2; case 4: return AVG_4;
        case 8: return AVG_8; case 16: return AVG_16; case 32: return AVG_32;
        default: return AVG_4;
    }
}

bool max3010x_write_reg(max3010x_t *dev, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return i2c_write_blocking(dev->i2c, dev->address, buf, 2, false) == 2;
}
bool max3010x_read_reg(max3010x_t *dev, uint8_t reg, uint8_t *val) {
    if (i2c_write_blocking(dev->i2c, dev->address, &reg, 1, true) != 1) return false;
    return i2c_read_blocking(dev->i2c, dev->address, val, 1, false) == 1;
}
bool max3010x_bitmask(max3010x_t *dev, uint8_t reg, uint8_t mask, uint8_t thing) {
    uint8_t orig = 0;
    if (!max3010x_read_reg(dev, reg, &orig)) return false;
    orig &= mask;           // clear
    orig |= thing;          // set
    return max3010x_write_reg(dev, reg, orig);
}

static bool soft_reset(max3010x_t *dev) {
    if (!max3010x_bitmask(dev, REG_MODECONFIG, RESET_MASK, RESET_BIT)) return false;
    // wait up to 100ms for reset to clear
    absolute_time_t start = get_absolute_time();
    while (absolute_time_diff_us(start, get_absolute_time()) < 100000) {
        uint8_t v = 0;
        if (!max3010x_read_reg(dev, REG_MODECONFIG, &v)) return false;
        if ((v & RESET_BIT) == 0) return true;
        sleep_ms(1);
    }
    return false;
}

static bool check_part_id(max3010x_t *dev) {
    uint8_t part = 0;
    if (!max3010x_read_reg(dev, REG_PARTID, &part)) return false;
    return part == 0x15; // MAX30102
}

bool max3010x_clear_fifo(max3010x_t *dev) {
    return max3010x_write_reg(dev, REG_FIFOWRITEPTR, 0) &&
           max3010x_write_reg(dev, REG_FIFOOVERFLOW, 0) &&
           max3010x_write_reg(dev, REG_FIFOREADPTR, 0);
}

bool max3010x_init(max3010x_t *dev) {
    dev->i2c = I2C_PORT;
    dev->address = MAX3010X_I2C_ADDR;
    dev->active_leds = 2; // red+ir

    i2c_init(dev->i2c, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    if (!check_part_id(dev)) return false;
    if (!soft_reset(dev)) return false;
    return true;
}

bool max3010x_configure_spo2(max3010x_t *dev,
                             uint8_t led_brightness,
                             uint8_t sample_average,
                             int sample_rate,
                             int pulse_width,
                             int adc_range) {
    // FIFO: set averaging and rollover
    if (!max3010x_bitmask(dev, REG_FIFOCONFIG, SAMPLEAVG_MASK, map_avg(sample_average))) return false;
    if (!max3010x_bitmask(dev, REG_FIFOCONFIG, ROLLOVER_MASK, ROLLOVER_ENABLE)) return false;

    // Mode: Red+IR
    if (!max3010x_bitmask(dev, REG_MODECONFIG, MODE_MASK, MODE_REDIRONLY)) return false;

    // SPO2 config: ADC range, sample rate, pulse width
    if (!max3010x_bitmask(dev, REG_PARTICLECONFIG, ADCRANGE_MASK, map_adc_range(adc_range))) return false;
    if (!max3010x_bitmask(dev, REG_PARTICLECONFIG, SAMPLERATE_MASK, map_sample_rate(sample_rate))) return false;
    if (!max3010x_bitmask(dev, REG_PARTICLECONFIG, PWIDTH_MASK, map_pwidth(pulse_width))) return false;

    // LED amplitudes
    if (!max3010x_write_reg(dev, REG_LED1_PULSEAMP, led_brightness)) return false; // Red
    if (!max3010x_write_reg(dev, REG_LED2_PULSEAMP, led_brightness)) return false; // IR

    // Multi-LED slots (ch1=red, ch2=ir)
    if (!max3010x_bitmask(dev, REG_MULTILEDCONFIG1, SLOT1_MASK, SLOT_RED_LED)) return false;
    if (!max3010x_bitmask(dev, REG_MULTILEDCONFIG1, SLOT2_MASK, (uint8_t)(SLOT_IR_LED << 4))) return false;

    return max3010x_clear_fifo(dev);
}

bool max3010x_read_sample(max3010x_t *dev, max3010x_sample_t *out) {
    // Check if data is available: compare read vs write pointers
    uint8_t rd = 0, wr = 0;
    if (!max3010x_read_reg(dev, REG_FIFOREADPTR, &rd)) return false;
    if (!max3010x_read_reg(dev, REG_FIFOWRITEPTR, &wr)) return false;
    while (rd == wr) {
        // no new data yet; small sleep to avoid busy loop
        sleep_ms(1);
        if (!max3010x_read_reg(dev, REG_FIFOREADPTR, &rd)) return false;
        if (!max3010x_read_reg(dev, REG_FIFOWRITEPTR, &wr)) return false;
    }

    // Set register pointer to FIFO data and read 6 bytes (Red 3B + IR 3B)
    uint8_t reg = REG_FIFODATA;
    if (i2c_write_blocking(dev->i2c, dev->address, &reg, 1, true) != 1) return false;

    uint8_t buf[6] = {0};
    if (i2c_read_blocking(dev->i2c, dev->address, buf, 6, false) != 6) return false;

    // 18-bit values in 3 bytes. Mask to 18 bits.
    uint32_t red = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    uint32_t ir  = ((uint32_t)buf[3] << 16) | ((uint32_t)buf[4] << 8) | buf[5];

    out->red = red & 0x3FFFF;
    out->ir  = ir  & 0x3FFFF;
    return true;
}
