/**
 * @file task_temp_dist.h
 * @brief Task de medição de temperatura e distância
 * 
 * Esta task controla sensores de temperatura infravermelha (MLX90614)
 * e distância a laser (VL53L0X) para realizar medições precisas de
 * temperatura corporal com controle de posicionamento adequado.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#ifndef TASK_TEMP_DIST_H
#define TASK_TEMP_DIST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusões necessárias */
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "queue.h"
#include "task_fsm.h"
#include "lib/sensor/mlx90614/mlx90614.h"
#include "lib/sensor/vl53l0x/vl53l0x.h"
#include <stdio.h>
#include <math.h>

/* Definições e constantes */
#define I2C_PORT i2c1                          /**< Porta I2C utilizada */
#define I2C_SDA_PIN 2                          /**< Pino SDA do I2C */
#define I2C_SCL_PIN 3                          /**< Pino SCL do I2C */
#define SENSOR_CALIBRATION_OFFSET_MM 37        /**< Offset de calibração distância */
#define IDEAL_MIN_DIST_MM 30                   /**< Distância mínima ideal */
#define IDEAL_MAX_DIST_MM 50                   /**< Distância máxima ideal */
#define TEMP_OFFSET_C 4.3f                     /**< Offset de calibração temperatura */
#define SENSOR_NUM_SAMPLES 50                  /**< Número de amostras para média */
#define SENSOR_DELAY_BETWEEN_SAMPLES_MS 20     /**< Delay entre amostras */
#define TEMP_WINDOW_MS 10000                   /**< Janela temporal para medição */

/* Variáveis externas compartilhadas */
extern EventGroupHandle_t xEvents;            /**< Grupo de eventos FreeRTOS */
extern QueueHandle_t xUICommandQueue;         /**< Fila de comandos UI */
extern QueueHandle_t xAudioCommandQueue;      /**< Fila de comandos áudio */
extern estado_t estado_atual;                 /**< Estado atual da FSM */
extern paciente_t pacienteAtual;              /**< Dados do paciente atual */
extern bool temp_initialized;                 /**< Flag de inicialização dos sensores */

/* Declarações de função */

/**
 * @brief Task de medição de temperatura e distância
 * @param pvParameters Parâmetros da task (não utilizado)
 * 
 * Esta task controla sensores MLX90614 e VL53L0X para medir
 * temperatura corporal com validação de distância adequada.
 */
void TaskTempDist(void *pvParameters);

/**
 * @brief Calibra leitura de temperatura
 * @param t_obj_c Temperatura do objeto em Celsius
 * @param t_amb_c Temperatura ambiente em Celsius
 * @return Temperatura calibrada em Celsius
 * 
 * Aplica correções baseadas em temperatura ambiente e offsets
 * empíricos para melhorar precisão da medição.
 */
float calibrate_temp(float t_obj_c, float t_amb_c);

#ifdef __cplusplus
}
#endif

#endif // TASK_TEMP_DIST_H
