/**
 * @file task_audio.h
 * @brief Task responsável pelo controle de reprodução de áudio
 * 
 * Esta task gerencia a reprodução de arquivos de áudio durante o processo
 * de triagem, processando comandos de play/stop e mantendo o sistema I2S
 * funcionando adequadamente.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#ifndef TASK_AUDIO_H
#define TASK_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusões necessárias */
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "task_fsm.h"
#include "../drivers/audio.h"
#include "ff.h"
#include <stdio.h>

/* Variáveis externas compartilhadas */
extern QueueHandle_t xAudioCommandQueue;    /**< Fila de comandos de áudio */
extern audio_buffer_pool_t *producer_pool;  /**< Pool de buffers I2S */

/* Declaração da função principal da task */

/**
 * @brief Task de controle de reprodução de áudio
 * @param pvParameters Parâmetros da task (não utilizado)
 * 
 * Esta task fica em loop processando comandos de áudio e mantendo
 * o sistema I2S alimentado com dados de amostras.
 */
void TaskAudio(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif // TASK_AUDIO_H
