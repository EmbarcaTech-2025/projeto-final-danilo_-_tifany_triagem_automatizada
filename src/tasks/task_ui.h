/**
 * @file task_ui.h
 * @brief Task de interface do usuário com display Nextion
 * 
 * Esta task gerencia toda a comunicação com o display Nextion, processando
 * comandos da FSM para atualização da interface e capturando eventos de
 * toque do usuário.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#ifndef TASK_UI_H
#define TASK_UI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusões necessárias */
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"
#include "task_fsm.h"
#include "../drivers/nextion.h"
#include "../drivers/wifi_client.h"
#include <stdio.h>
#include <string.h>

/* Definições do protocolo Nextion */
#define NEXTION_STARTUP_HEADER      0x00   /**< Header de inicialização */
#define NEXTION_READY_HEADER        0x88   /**< Header de display pronto */
#define NEXTION_TOUCH_EVENT_HEADER  0x65   /**< Header de evento de toque */
#define NEXTION_END_OF_MESSAGE      0xFF   /**< Marcador fim de mensagem */

/* Configuração UART */
#define UART_ID uart0                      /**< Instância UART utilizada */

/* Variáveis externas compartilhadas */
extern EventGroupHandle_t xEvents;         /**< Grupo de eventos */
extern QueueHandle_t xUICommandQueue;     /**< Fila de comandos UI */
extern paciente_t pacienteAtual;           /**< Dados do paciente atual */

/* Declaração da função principal da task */

/**
 * @brief Task de gerenciamento da interface do usuário
 * @param pvParameters Parâmetros da task (não utilizado)
 * 
 * Esta task processa comandos para atualizar a interface e monitora
 * eventos de toque do usuário no display Nextion.
 */
void TaskUI(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // TASK_UI_H
