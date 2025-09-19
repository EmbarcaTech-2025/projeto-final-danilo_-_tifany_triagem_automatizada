/**
 * @file task_wifi_check.c
 * @brief Implementação da task de verificação WiFi
 * 
 * Esta task executa uma vez durante a inicialização para estabelecer
 * conexão WiFi. Se falhar, o sistema continua em modo offline.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#include "task_wifi_check.h"

/**
 * @brief Task de inicialização e verificação WiFi
 * @param pvParameters Parâmetros FreeRTOS (não utilizado)
 * 
 * Tenta conectar ao WiFi por até 30 segundos. Após completar
 * (com sucesso ou falha), a task se auto-deleta.
 */
void TaskWiFiCheck(void *pvParameters)
{
    printf("Iniciando WiFi...\n");
    if (!wifi_wait_connected(30000))
    {
        printf("AVISO: WiFi não conectou, continuando offline...\n");
    }
    else
    {
        printf("WiFi conectado!\n");
    }
    vTaskDelete(NULL);
}
