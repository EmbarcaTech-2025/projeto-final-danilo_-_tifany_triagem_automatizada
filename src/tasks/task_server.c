/**
 * @file task_server.c
 * @brief Implementação da task de armazenamento local
 * 
 * Esta task mantém um buffer circular local para armazenar
 * dados de pacientes, útil quando não há conectividade remota.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#include "task_server.h"

/**
 * @brief Task de servidor local (buffer circular)
 * @param pvParameters Parâmetros FreeRTOS (não utilizado)
 * 
 * Recebe dados de pacientes da fila xDataQueue e os armazena
 * em buffer local de 20 posições com comportamento circular.
 */
void TaskServer(void *pvParameters)
{
    static paciente_t buffer[20];  /* Buffer circular local */
    static int idx = 0;            /* Índice de inserção */
    
    while (true)
    {
        paciente_t p;
        if (xQueueReceive(xDataQueue, &p, portMAX_DELAY))
        {
            buffer[idx++] = p;
            if (idx >= 20)
                idx = 0;  /* Wraparound circular */
            printf("[SERVER] Paciente salvo no buffer local (idx=%d)\n", idx);
        }
    }
}
