/**
 * @file task_hooks.h
 * @brief Hooks de depuração e segurança do FreeRTOS
 * 
 * Este módulo implementa funções de callback (hooks) do FreeRTOS
 * para detectar problemas críticos como overflow de stack e
 * falhas de alocação de memória.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#ifndef TASK_HOOKS_H
#define TASK_HOOKS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Inclusões necessárias */
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/* Declarações de funções hook */

/**
 * @brief Hook chamado quando há overflow de stack em uma task
 * @param xTask Handle da task que teve overflow
 * @param pcTaskName Nome da task que teve overflow
 * 
 * Esta função é chamada automaticamente pelo FreeRTOS quando
 * detecta overflow de stack. Imprime mensagem de erro e trava
 * o sistema para análise.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName);

/**
 * @brief Hook chamado quando falha alocação de memória
 * 
 * Esta função é chamada automaticamente pelo FreeRTOS quando
 * não consegue alocar memória. Imprime mensagem de erro e trava
 * o sistema para análise.
 */
void vApplicationMallocFailedHook(void);

#ifdef __cplusplus
}
#endif

#endif // TASK_HOOKS_H
