/**
 * @file task_fsm.h
 * @brief Máquina de Estados Finitos principal do sistema de triagem
 * 
 * Este módulo implementa a FSM (Finite State Machine) que controla o fluxo
 * principal do sistema de triagem, coordenando as diferentes etapas do
 * atendimento desde a tela inicial até o envio dos dados.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#ifndef TASK_FSM_H
#define TASK_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusões necessárias */
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include "../utils/json_logger.h"
#include "../drivers/wifi_client.h"
#include "../drivers/nextion.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Estados da máquina de estados finitos */
typedef enum
{
    ESTADO_INICIAL,     /**< Tela inicial aguardando toque */
    ESTADO_AVISO,       /**< Exibição de avisos importantes */
    ESTADO_LGPD,        /**< Termo LGPD para aceite */
    ESTADO_CADASTRO,    /**< Cadastro de dados pessoais */
    ESTADO_ALERTAS,     /**< Captura de alertas críticos */
    ESTADO_SINTOMAS,    /**< Seleção de sintomas */
    ESTADO_PPG,         /**< Medição de parâmetros cardíacos */
    ESTADO_TEMP,        /**< Medição de temperatura */
    ESTADO_RESULTADOS,  /**< Exibição dos resultados */
    ESTADO_FINAL        /**< Finalização do atendimento */
} estado_t;

/* Tipos de comandos para comunicação entre tasks */
typedef enum
{
    /* Comandos de interface */
    UI_CMD_SHOW_PAGE,      /**< Exibir página no display */
    UI_CMD_SHOW_TEXT,      /**< Mostrar texto */
    UI_CMD_UPDATE_TEXT,    /**< Atualizar texto existente */
    UI_CMD_PLAY_SOUND,     /**< Reproduzir som */
    UI_CMD_UPDATE_PROGRESS,/**< Atualizar barra de progresso */
    
    /* Comandos de áudio */
    AUDIO_CMD_PLAY,        /**< Iniciar reprodução */
    AUDIO_CMD_STOP,        /**< Parar reprodução */
} command_type_t;

/* Estrutura de comando para comunicação entre tasks */
typedef struct
{
    command_type_t cmd;    /**< Tipo do comando */
    uint8_t param1;        /**< Parâmetro 1 (página, track_id, etc) */
    char text[64];         /**< Texto para interface */
    char filename[32];     /**< Nome do arquivo de áudio */
} task_command_t;

/* Variáveis globais exportadas */
extern uint32_t numero_atendimento;    /**< Contador de atendimentos */
extern estado_t estado_atual;          /**< Estado atual da FSM */
extern paciente_t pacienteAtual;       /**< Dados do paciente atual */

/* Bits de eventos (redefinidos aqui por compatibilidade) */
#define EVT_PPG_READY (1 << 0)        /**< Dados PPG prontos */
#define EVT_TEMP_READY (1 << 1)       /**< Dados temperatura prontos */
#define EVT_UI_TOUCH_START (1 << 2)   /**< Toque inicial */
#define EVT_UI_TOUCH_NEXT (1 << 3)    /**< Próximo passo */
#define EVT_UI_TOUCH_FINISH (1 << 4)  /**< Finalizar */
#define EVT_AUDIO_FINISHED (1 << 5)   /**< Áudio terminado */
#define EVT_LGPD_ACCEPTED (1 << 6)    /**< LGPD aceita */
#define EVT_LGPD_REJECTED (1 << 7)    /**< LGPD rejeitada */

/* Handles de recursos FreeRTOS externos */
extern EventGroupHandle_t xEvents;           /**< Grupo de eventos */
extern QueueHandle_t xDataQueue;            /**< Fila de dados */
extern QueueHandle_t xUICommandQueue;       /**< Fila de comandos UI */
extern QueueHandle_t xAudioCommandQueue;    /**< Fila de comandos áudio */
extern SemaphoreHandle_t xI2CMutex;         /**< Mutex para acesso I2C */

/* Declaração da função principal da task FSM */

/**
 * @brief Task principal da máquina de estados finitos
 * @param pvParameters Parâmetros da task (não utilizado)
 */
void TaskFSM(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // TASK_FSM_H
