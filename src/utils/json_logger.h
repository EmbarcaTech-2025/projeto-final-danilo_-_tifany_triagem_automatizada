/**
 * @file json_logger.h
 * @brief Sistema de logging em formato JSON para dados dos pacientes
 * 
 * Este módulo fornece funcionalidades para salvar os dados dos pacientes
 * em arquivos JSON estruturados no cartão SD, permitindo posterior análise
 * e backup dos atendimentos realizados.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#ifndef JSON_LOGGER_H
#define JSON_LOGGER_H

#include "ff.h"
#include "../drivers/wifi_client.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Salva dados do paciente em arquivo de log JSON
 * @param paciente Ponteiro para estrutura de dados do paciente
 * @return true se salvamento bem-sucedido, false caso contrário
 */
bool save_patient_to_json_log(const paciente_t* paciente);

/**
 * @brief Obtém timestamp atual formatado para logs
 * @param buffer Buffer para armazenar o timestamp
 * @param buffer_size Tamanho do buffer fornecido
 */
void get_timestamp_string(char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // JSON_LOGGER_H
