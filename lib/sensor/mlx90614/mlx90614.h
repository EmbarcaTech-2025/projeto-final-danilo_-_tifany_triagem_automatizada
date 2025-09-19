#pragma once
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Endereço padrão do MLX90614
#define MLX90614_DEFAULT_ADDR 0x5A

// Registradores RAM
#define MLX90614_REG_TA     0x06  // Ambient
#define MLX90614_REG_TOBJ1  0x07  // Object 1

// Inicializa o driver com a instância do I2C e o endereço
// Retorna 1 em sucesso, 0 em falha
int  mlx90614_init(i2c_inst_t *i2c, uint8_t addr);

// Lê a temperatura ambiente em Celsius; retorna true em sucesso
bool mlx90614_read_ambient_c(float *temp_c);

// Lê a temperatura do objeto (corpo) em Celsius; retorna true em sucesso
bool mlx90614_read_object_c(float *temp_c);
