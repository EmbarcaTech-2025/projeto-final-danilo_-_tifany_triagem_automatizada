/**
 * @file task_wifi_check.h
 * @brief Task de verificação e inicialização da conexão WiFi
 * 
 * Esta task é responsável por estabelecer a conexão WiFi durante a
 * inicialização do sistema, permitindo comunicação com servidores
 * externos quando necessário.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#ifndef TASK_WIFI_CHECK_H
#define TASK_WIFI_CHECK_H

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusões necessárias */
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "../drivers/wifi_client.h"
#include <stdio.h>

/* Declaração da função principal da task */

/**
 * @brief Task de verificação e inicialização WiFi
 * @param pvParameters Parâmetros da task (não utilizado)
 * 
 * Esta task tenta estabelecer conexão WiFi por até 30 segundos.
 * Se não conseguir, o sistema continua funcionando offline.
 */
void TaskWiFiCheck(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // TASK_WIFI_CHECK_H
