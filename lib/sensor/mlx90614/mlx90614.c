#include "mlx90614.h"

static i2c_inst_t *mlx_i2c = NULL;
static uint8_t mlx_addr = MLX90614_DEFAULT_ADDR;

// Leitura SMBus Word (LSB, MSB, PEC). Ignora PEC para simplicidade.
static bool mlx90614_read_word(uint8_t reg, uint16_t *out)
{
    if (!mlx_i2c || !out) return false;

    // Envia o registrador com RESTART (no stop)
    int w = i2c_write_blocking(mlx_i2c, mlx_addr, &reg, 1, true);
    if (w != 1) return false;

    // Lê 3 bytes: LSB, MSB, PEC
    uint8_t buf[3] = {0};
    int r = i2c_read_blocking(mlx_i2c, mlx_addr, buf, 3, false);
    if (r != 3) return false;

    // Concatena MSB/LSB
    *out = ((uint16_t)buf[1] << 8) | buf[0];
    return true;
}

int mlx90614_init(i2c_inst_t *i2c, uint8_t addr)
{
    mlx_i2c = i2c;
    mlx_addr = addr;

    // Pequeno atraso e tentativas (MLX pode precisar de tempo após power-up)
    sleep_ms(50);
    uint16_t raw = 0;
    for (int i = 0; i < 5; i++) {
        if (mlx90614_read_word(MLX90614_REG_TA, &raw)) {
            return 1;
        }
        sleep_ms(20);
    }
    return 0;
}

static inline float mlx_raw_to_c(uint16_t raw)
{
    // Conversão: Kelvin = raw * 0.02; Celsius = Kelvin - 273.15
    float kelvin = (float)(raw & 0x7FFF) * 0.02f;
    return kelvin - 273.15f;
}

bool mlx90614_read_ambient_c(float *temp_c)
{
    if (!temp_c) return false;
    uint16_t raw = 0;
    if (!mlx90614_read_word(MLX90614_REG_TA, &raw)) return false;
    *temp_c = mlx_raw_to_c(raw);
    return true;
}

bool mlx90614_read_object_c(float *temp_c)
{
    if (!temp_c) return false;
    uint16_t raw = 0;
    if (!mlx90614_read_word(MLX90614_REG_TOBJ1, &raw)) return false;
    *temp_c = mlx_raw_to_c(raw);
    return true;
}
