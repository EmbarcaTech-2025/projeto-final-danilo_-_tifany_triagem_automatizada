/**
 * @file task_server.h
 * @brief Task de armazenamento local de dados
 * 
 * Esta task implementa um buffer local para armazenar dados de
 * pacientes quando não há conectividade com servidor remoto,
 * garantindo que informações não sejam perdidas.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#ifndef TASK_SERVER_H
#define TASK_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusões necessárias */
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "task_fsm.h"
#include <stdio.h>

/* Variáveis externas compartilhadas */
extern QueueHandle_t xDataQueue;    /**< Fila de dados de pacientes */

/* Declaração da função principal da task */

/**
 * @brief Task de armazenamento local de dados
 * @param pvParameters Parâmetros da task (não utilizado)
 * 
 * Esta task recebe dados de pacientes via fila e os armazena
 * em buffer circular local de até 20 entradas.
 */
void TaskServer(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // TASK_SERVER_H
