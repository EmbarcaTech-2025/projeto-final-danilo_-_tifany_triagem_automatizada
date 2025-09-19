/**
 * @file task_hooks.c
 * @brief Implementação dos hooks de depuração do FreeRTOS
 * 
 * Este arquivo implementa as funções de callback do FreeRTOS
 * para detecção de problemas críticos durante a execução.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#include "task_hooks.h"

/**
 * @brief Hook executado quando detectado overflow de stack
 * @param xTask Handle da task problemática
 * @param pcTaskName Nome da task problemática
 * 
 * Função chamada automaticamente pelo FreeRTOS quando uma task
 * excede o limite de stack alocado. Imprime diagnóstico e trava.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("*** STACK OVERFLOW *** Task: %s\n", pcTaskName);
    while (1) {
        tight_loop_contents();
    }
}

/**
 * @brief Hook executado quando falha alocação de memória
 * 
 * Função chamada automaticamente pelo FreeRTOS quando não há
 * memória heap suficiente para alocação. Imprime diagnóstico e trava.
 */
void vApplicationMallocFailedHook(void) {
    printf("*** MALLOC FAILED *** Out of heap memory!\n");
    while (1) {
        tight_loop_contents();
    }
}
