/**
 * @file task_audio.c
 * @brief Implementação da task de controle de reprodução de áudio
 * 
 * Esta task gerencia o sistema I2S para reprodução de arquivos WAV
 * durante o processo de triagem, processando comandos de play/stop
 * e mantendo o stream de áudio funcionando continuamente.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#include "task_audio.h"

/* Variáveis estáticas para controle de arquivos WAV */
static FIL wav_file;           /**< Handle do arquivo WAV atual */
static bool wav_file_open = false;  /**< Flag indicando arquivo aberto */

/**
 * @brief Task principal de controle de áudio
 * @param pvParameters Parâmetros FreeRTOS (não utilizado)
 * 
 * Esta task processa comandos de áudio via fila e mantém o sistema
 * I2S alimentado continuamente com dados de áudio. Implementa timing
 * crítico de 1ms para evitar buffer underruns.
 */
void TaskAudio(void *pvParameters)
{
    printf("[AUDIO] Task iniciada\n");

    /* Aguarda estabilização de outras tasks */
    vTaskDelay(pdMS_TO_TICKS(500));

    if (!producer_pool)
    {
        printf("[AUDIO] AVISO: Sistema de áudio desabilitado\n");
        /* Loop simples processando comandos sem reprodução */
        while (true)
        {
            task_command_t cmd;
            if (xQueueReceive(xAudioCommandQueue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE)
            {
                printf("[AUDIO] Comando ignorado (áudio desabilitado)\n");
            }
        }
    }

    printf("[AUDIO] Sistema ativo\n");
    TickType_t last_wake = xTaskGetTickCount();

    while (true)
    {
        /* 1. Alimenta I2S continuamente (timing crítico) */
        audio_service_once();

        /* 2. Processa comandos de áudio (não-bloqueante) */
        task_command_t cmd;
        if (xQueueReceive(xAudioCommandQueue, &cmd, 0) == pdTRUE)
        {
            switch (cmd.cmd)
            {
            case AUDIO_CMD_PLAY:
                printf("[AUDIO] Play: %s\n", cmd.filename);
                play_audio_file(cmd.filename);
                break;

            case AUDIO_CMD_STOP:
                printf("[AUDIO] Stop\n");
                wav_playing = false;
                if (wav_file_open)
                {
                    f_close(&wav_file);
                    wav_file_open = false;
                }
                break;

            default:
                break;
            }
        }

        /* 3. Timing crítico - 1ms para evitar buffer underruns */
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1));
    }
}
