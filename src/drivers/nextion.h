/**
 * @file nextion.h
 * @brief Driver para controle de display Nextion via UART
 * 
 * Este módulo fornece interface para comunicação com displays Nextion,
 * incluindo envio de comandos, captura de dados de formulários e
 * processamento de eventos de toque.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Declarações das funções da API Nextion */

/**
 * @brief Envia comando para o display Nextion
 * @param cmd String contendo o comando a ser enviado
 */
void nextion_send_command(const char *cmd);

/**
 * @brief Inicializa a comunicação UART com o display Nextion
 */
void nextion_init(void);

/**
 * @brief Captura sintomas selecionados nos checkboxes do display
 * 
 * Lê os valores dos checkboxes de sintomas na interface do usuário
 * e armazena a string resultante no paciente atual.
 */
void capturar_sintomas(void);

/**
 * @brief Captura alertas críticos selecionados no display
 * 
 * Lê os valores dos checkboxes de alertas críticos na interface
 * e armazena a informação no paciente atual.
 */
void capturar_alertas(void);

#ifdef __cplusplus
}
#endif