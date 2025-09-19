/**
 * @file task_ppg.h
 * @brief Task de medição de parâmetros cardíacos via sensor PPG
 * 
 * Esta task gerencia o sensor MAX30102 para capturar dados de
 * fotopletismografia (PPG), calculando batimentos cardíacos,
 * saturação de oxigênio e estimativa de pressão arterial.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#ifndef TASK_PPG_H
#define TASK_PPG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusões necessárias */
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "task_fsm.h"
#include "lib/sensor/max3010x/max3010x.h"
#include "lib/sensor/max3010x/spo2_algorithm.h"
#include "lib/sensor/max3010x/ppg_blood_pressure.h"
#include "lib/sensor/max3010x/heart_rate.h"
#include <stdio.h>

/* Variáveis externas compartilhadas */
extern EventGroupHandle_t xEvents;         /**< Grupo de eventos FreeRTOS */
extern QueueHandle_t xUICommandQueue;      /**< Fila de comandos para UI */
extern estado_t estado_atual;              /**< Estado atual da FSM */
extern paciente_t pacienteAtual;           /**< Dados do paciente atual */

/* Declaração da função principal da task */

/**
 * @brief Task de medição de parâmetros cardíacos
 * @param pvParameters Parâmetros da task (não utilizado)
 * 
 * Esta task controla o sensor MAX30102 para capturar sinais PPG,
 * processar os dados e calcular BPM, SpO2 e estimativa de pressão.
 */
void TaskPPG(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // TASK_PPG_H
