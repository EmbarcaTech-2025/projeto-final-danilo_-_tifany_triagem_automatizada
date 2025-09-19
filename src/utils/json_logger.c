/**
 * @file json_logger.c
 * @brief Implementação do sistema de logging JSON
 * 
 * Este arquivo implementa as funções para criar e manter arquivos de log
 * em formato JSON contendo os dados dos pacientes atendidos pelo sistema
 * de triagem, incluindo informações de data/hora e dados vitais.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#include "json_logger.h"
#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "pico/util/datetime.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Configurações do sistema de logging */
#define LOG_FILENAME "triagem_log.json"  /**< Nome do arquivo de log */
#define MAX_JSON_ENTRY_SIZE 512         /**< Tamanho máximo de entrada JSON */
#define TEMP_BUFFER_SIZE 1024           /**< Tamanho do buffer temporário */

void get_timestamp_string(char* buffer, size_t buffer_size) {
    datetime_t dt;
    
    /* Tenta obter horário do RTC */
    if (rtc_get_datetime(&dt)) {
        snprintf(buffer, buffer_size, "%04d-%02d-%02d %02d:%02d:%02d",
                dt.year, dt.month, dt.day, dt.hour, dt.min, dt.sec);
    } else {
        /* Fallback usando tempo de boot se RTC indisponível */
        uint32_t ms = to_ms_since_boot(get_absolute_time());
        uint32_t seconds = ms / 1000;
        uint32_t hours = seconds / 3600;
        uint32_t minutes = (seconds % 3600) / 60;
        uint32_t secs = seconds % 60;
        
        snprintf(buffer, buffer_size, "BOOT+%02lu:%02lu:%02lu", 
                (unsigned long)hours, (unsigned long)minutes, (unsigned long)secs);
    }
}

// Function to check if file exists
static bool file_exists(const char* filename) {
    FIL test_file;
    FRESULT result = f_open(&test_file, filename, FA_READ);
    if (result == FR_OK) {
        f_close(&test_file);
        return true;
    }
    return false;
}

// Function to create new JSON log file with initial structure
static bool create_new_json_log(void) {
    FIL log_file;
    FRESULT result;
    UINT bytes_written;
    
    result = f_open(&log_file, LOG_FILENAME, FA_CREATE_ALWAYS | FA_WRITE);
    if (result != FR_OK) {
        printf("[JSON] ERRO: Falha ao criar arquivo de log: %d\n", result);
        return false;
    }
    
    // Write initial JSON structure
    const char* initial_json = "{\n  \"log_info\": {\n    \"created\": \"%s\",\n    \"description\": \"Sistema de Triagem - Log de Pacientes\"\n  },\n  \"patients\": [\n  ]\n}\n";
    
    char timestamp[32];
    get_timestamp_string(timestamp, sizeof(timestamp));
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), initial_json, timestamp);
    
    result = f_write(&log_file, buffer, strlen(buffer), &bytes_written);
    f_close(&log_file);
    
    if (result != FR_OK || bytes_written != strlen(buffer)) {
        printf("[JSON] ERRO: Falha ao escrever estrutura inicial: %d\n", result);
        return false;
    }
    
    printf("[JSON] ✅ Arquivo de log criado com sucesso\n");
    return true;
}

// Function to append patient data to existing JSON log
static bool append_patient_to_json(const paciente_t* paciente) {
    FIL log_file;
    FRESULT result;
    UINT bytes_read, bytes_written;
    char* file_buffer = NULL;
    FSIZE_t file_size;
    bool success = false;
    
    // Open file for reading
    result = f_open(&log_file, LOG_FILENAME, FA_READ);
    if (result != FR_OK) {
        printf("[JSON] ERRO: Falha ao abrir arquivo para leitura: %d\n", result);
        return false;
    }
    
    // Get file size
    file_size = f_size(&log_file);
    if (file_size == 0 || file_size > 32768) { // Limit to 32KB for safety
        printf("[JSON] ERRO: Tamanho de arquivo inválido: %lu\n", (unsigned long)file_size);
        f_close(&log_file);
        return false;
    }
    
    // Allocate buffer for file content
    file_buffer = malloc((size_t)file_size + MAX_JSON_ENTRY_SIZE + 100);
    if (!file_buffer) {
        printf("[JSON] ERRO: Falha ao alocar memória\n");
        f_close(&log_file);
        return false;
    }
    
    // Read entire file
    result = f_read(&log_file, file_buffer, (UINT)file_size, &bytes_read);
    f_close(&log_file);
    
    if (result != FR_OK || bytes_read != file_size) {
        printf("[JSON] ERRO: Falha ao ler arquivo: %d\n", result);
        free(file_buffer);
        return false;
    }
    
    // Null terminate the buffer
    file_buffer[bytes_read] = '\0';
    
    // Find the last ] in the patients array to insert new entry
    char* insert_pos = strstr(file_buffer, "  ]\n}");
    if (!insert_pos) {
        printf("[JSON] ERRO: Estrutura JSON inválida\n");
        free(file_buffer);
        return false;
    }
    
    // Get timestamp
    char timestamp[32];
    get_timestamp_string(timestamp, sizeof(timestamp));
    
    // Check if this is the first patient entry
    char* patients_start = strstr(file_buffer, "\"patients\": [");
    bool is_first_entry = true;
    if (patients_start) {
        char* check_pos = patients_start + strlen("\"patients\": [");
        while (*check_pos && (*check_pos == '\n' || *check_pos == ' ' || *check_pos == '\t')) {
            check_pos++;
        }
        if (*check_pos != ']') {
            is_first_entry = false;
        }
    }
    
    // Create new patient entry JSON
    char patient_json[MAX_JSON_ENTRY_SIZE];
    snprintf(patient_json, sizeof(patient_json),
        "%s    {\n"
        "      \"timestamp\": \"%s\",\n"
        "      \"cpf\": \"%s\",\n"
        "      \"sintomas\": \"%s\",\n"
        "      \"medidas\": {\n"
        "        \"bpm\": %d,\n"
        "        \"spo2\": %d,\n"
        "        \"temperatura\": %.1f,\n"
        "        \"pressao_sistolica\": %d,\n"
        "        \"pressao_diastolica\": %d,\n"
        "        \"distancia_mm\": %d\n"
        "      },\n"
        "      \"status\": \"%s\"\n"
        "    }",
        is_first_entry ? "\n" : ",\n",
        timestamp,
        paciente->cpf,
        paciente->sintomas,
        paciente->bpm,
        paciente->spo2,
        paciente->temperatura,
        (int)paciente->pressao_sys,
        (int)paciente->pressao_dia,
        paciente->distancia_mm,
        paciente->valid ? "completo" : "incompleto"
    );
    
    // Calculate new file size
    size_t prefix_len = insert_pos - file_buffer;
    size_t suffix_len = strlen(insert_pos);
    size_t new_size = prefix_len + strlen(patient_json) + suffix_len;
    
    // Create new file content
    char* new_buffer = malloc(new_size + 1);
    if (!new_buffer) {
        printf("[JSON] ERRO: Falha ao alocar memória para novo conteúdo\n");
        free(file_buffer);
        return false;
    }
    
    // Assemble new content
    memcpy(new_buffer, file_buffer, prefix_len);
    strcpy(new_buffer + prefix_len, patient_json);
    strcat(new_buffer + prefix_len, insert_pos);
    
    // Write new content to file
    result = f_open(&log_file, LOG_FILENAME, FA_CREATE_ALWAYS | FA_WRITE);
    if (result == FR_OK) {
        result = f_write(&log_file, new_buffer, strlen(new_buffer), &bytes_written);
        f_close(&log_file);
        
        if (result == FR_OK && bytes_written == strlen(new_buffer)) {
            printf("[JSON] ✅ Paciente adicionado ao log com sucesso\n");
            success = true;
        } else {
            printf("[JSON] ERRO: Falha ao escrever dados: %d\n", result);
        }
    } else {
        printf("[JSON] ERRO: Falha ao abrir arquivo para escrita: %d\n", result);
    }
    
    free(file_buffer);
    free(new_buffer);
    return success;
}

// Main function to save patient data to JSON log
bool save_patient_to_json_log(const paciente_t* paciente) {
    if (!paciente) {
        printf("[JSON] ERRO: Ponteiro de paciente nulo\n");
        return false;
    }
    
    printf("[JSON] Salvando dados do paciente %s no log...\n", paciente->cpf);
    
    // Check if log file exists
    if (!file_exists(LOG_FILENAME)) {
        printf("[JSON] Arquivo de log não existe, criando novo...\n");
        if (!create_new_json_log()) {
            return false;
        }
    }
    
    // Append patient data to existing log
    return append_patient_to_json(paciente);
}
