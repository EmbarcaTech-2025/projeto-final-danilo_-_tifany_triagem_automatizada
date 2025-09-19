/**
 * @file nextion.c
 * @brief Implementação do driver para display Nextion
 * 
 * Este arquivo contém a implementação das funções para comunicação com
 * displays Nextion, incluindo inicialização UART, envio de comandos e
 * captura de dados de formulários.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#include "nextion.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h" 
#include "event_groups.h"
#include "../tasks/task_fsm.h"
#include "wifi_client.h"

/* Configuração da comunicação UART com Nextion */
#define UART_ID         uart0           /**< Instância UART utilizada */
#define BAUD_RATE       9600           /**< Taxa de transmissão */
#define UART_TX_PIN     0              /**< Pino GPIO para transmissão */
#define UART_RX_PIN     1              /**< Pino GPIO para recepção */
#define NEXTION_END_OF_MESSAGE  0xFF   /**< Byte de fim de comando Nextion */

void nextion_init(void) {
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
}

void nextion_send_command(const char *cmd) {
    uart_puts(UART_ID, cmd);
    /* Envia 3 bytes de terminação conforme protocolo Nextion */
    uart_putc_raw(UART_ID, NEXTION_END_OF_MESSAGE);
    uart_putc_raw(UART_ID, NEXTION_END_OF_MESSAGE);
    uart_putc_raw(UART_ID, NEXTION_END_OF_MESSAGE);
}

void capturar_sintomas(void) {
    char sintomas[500] = "";
    bool primeiro = true;
    
    printf("[UI] 🔍 Iniciando captura de sintomas...\n");
    
    /* Array com informações dos checkboxes de sintomas */
    struct {
        const char *nome_cb;      /**< Nome do componente na Nextion */
        const char *descricao;    /**< Descrição do sintoma */
    } checkboxes[] = {
        {"cb_febre", "Febre (acima de 37.8°C)"},
        {"cb_fraq", "Mal-estar Geral / Fraqueza"},
        {"cb_calaf", "Calafrios ou Tremores"},
        {"cb_cabeca", "Dor de Cabeça"},
        {"cb_tont", "Tontura ou Vertigem"},
        {"cb_garg", "Dor de Garganta"},
        {"cb_gripe", "Nariz escorrendo ou entupido"},
        {"cb_tosse", "Tosse"},
        {"cb_abdom", "Dor Abdominal / na Barriga"},
        {"cb_vomito", "Náusea ou Vômito"},
        {"cb_diarreia", "Diarreia"},
        {"cb_azia", "Azia / Queimação no Estômago"},
        {"cb_corpo", "Dor no Corpo"},
        {"cb_ferimento", "Ferimento ou Queimadura Leve"},
        {"cb_urina", "Dor ou Ardência para Urinar"},
        {"cb_gineco", "Queixas Ginecológicas"}
    };
    
    /* Limpa buffer UART antes de iniciar comunicação */
    vTaskDelay(pdMS_TO_TICKS(100));
    while (uart_is_readable(UART_ID)) {
        uart_getc(UART_ID);
    }
    
    /* Processa cada checkbox (total de 16 sintomas) */
    for (int i = 0; i < 16; i++) {
        /* Constrói comando para ler valor do checkbox */
        char comando[32];
        sprintf(comando, "get %s.val", checkboxes[i].nome_cb);
        
        printf("[UI] Verificando %s: '%s'\n", checkboxes[i].descricao, comando);
        nextion_send_command(comando);
        vTaskDelay(pdMS_TO_TICKS(150)); // Delay maior
        
        // Variáveis para controle
        uint8_t valor = 0;
        bool valor_encontrado = false;
        
        // Lê todos os bytes disponíveis em um buffer
        uint8_t buffer[16] = {0};
        int idx = 0;
        uint32_t timeout = to_ms_since_boot(get_absolute_time()) + 300;
        
        while (to_ms_since_boot(get_absolute_time()) < timeout && uart_is_readable(UART_ID) && idx < 16) {
            buffer[idx] = uart_getc(UART_ID);
            printf("[UI] Byte %d: 0x%02X (%c)\n", idx, buffer[idx], 
                  (buffer[idx] >= 32 && buffer[idx] <= 126) ? buffer[idx] : '?');
            idx++;
        }
        
        // BUSCA O CÓDIGO 0x71 em QUALQUER POSIÇÃO DO BUFFER
        for (int j = 0; j < idx-4; j++) {
            if (buffer[j] == 0x71) {
                valor = buffer[j+1]; // O valor está no byte seguinte ao 0x71
                valor_encontrado = true;
                printf("[UI] ✅ Checkbox %s: valor=%d (encontrado na posição %d)\n", 
                      checkboxes[i].nome_cb, valor, j);
                break;
            }
        }
        
        // Tratamento especial para o primeiro checkbox (febre)
        if (!valor_encontrado && i == 0) {
            // Tentativa especial para Febre - verificar diretamente se está marcado
            nextion_send_command("get cb_febre.val");
            vTaskDelay(pdMS_TO_TICKS(200)); // Delay maior para Febre
            
            // Limpa buffer e lê novamente
            memset(buffer, 0, sizeof(buffer));
            idx = 0;
            timeout = to_ms_since_boot(get_absolute_time()) + 300;
            
            while (to_ms_since_boot(get_absolute_time()) < timeout && uart_is_readable(UART_ID) && idx < 16) {
                buffer[idx] = uart_getc(UART_ID);
                printf("[UI] Retry Febre Byte %d: 0x%02X\n", idx, buffer[idx]);
                idx++;
            }
            
            // Busca o código 0x71 novamente
            for (int j = 0; j < idx-4; j++) {
                if (buffer[j] == 0x71) {
                    valor = buffer[j+1];
                    valor_encontrado = true;
                    printf("[UI] ✅ Retry Febre: valor=%d\n", valor);
                    break;
                }
            }
        }
        
        // Se estiver marcado (valor = 1), adiciona à string
        if (valor == 1) {
            // Verifica se ainda há espaço no buffer
            if (strlen(sintomas) + strlen(checkboxes[i].descricao) + 3 < sizeof(sintomas)) {
                // Adiciona vírgula se não for o primeiro sintoma
                if (!primeiro) {
                    strcat(sintomas, ", ");
                } else {
                    primeiro = false;
                }
                
                // Adiciona o sintoma
                strcat(sintomas, checkboxes[i].descricao);
                printf("[UI] 📝 Adicionado sintoma: %s\n", checkboxes[i].descricao);
            } else {
                printf("[UI] ⚠️ Buffer de sintomas cheio, ignorando: %s\n", checkboxes[i].descricao);
            }
        }
        
        // Limpa buffer antes da próxima leitura
        while (uart_is_readable(UART_ID)) {
            uart_getc(UART_ID);
        }
        
        // Pausa entre checkboxes
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Se nenhum sintoma foi selecionado
    if (primeiro) {
        strcpy(sintomas, "Nenhum sintoma informado");
        printf("[UI] ℹ️ Nenhum sintoma selecionado\n");
    }
    
    // Armazena a string completa
    strcpy(pacienteAtual.sintomas, sintomas);
    printf("[UI] 📋 Sintomas capturados: %s\n", pacienteAtual.sintomas);
}

// Função para capturar alertas críticos selecionados
void capturar_alertas(void) {
    char alertas[200] = ""; // Buffer para construir a string de alertas
    bool primeiro = true;   // Flag para gerenciar as vírgulas
    bool tem_alerta = false; // Flag para verificar se tem algum alerta
    
    printf("[UI] 🚨 Iniciando captura de alertas críticos...\n");
    
    // Array com informações dos checkboxes de alerta
    struct {
        const char *nome_cb;  // Nome do componente na Nextion
        const char *descricao; // Descrição do alerta
    } checkboxes_alerta[] = {
        {"cb_dor", "DOR"},
        {"cb_neuro", "ESTADO NEUROLÓGICO"},
        {"cb_ar", "RESPIRAÇÃO"},
        {"cb_peito", "DOR NO PEITO"},
        {"cb_sang", "SANGRAMENTO"}
    };
    
    // Limpa COMPLETAMENTE a UART antes de começar
    vTaskDelay(pdMS_TO_TICKS(100));
    while (uart_is_readable(UART_ID)) {
        uart_getc(UART_ID);
    }
    
    // Processar cada checkbox de alerta
    for (int i = 0; i < 5; i++) {
        char comando[32];
        sprintf(comando, "get %s.val", checkboxes_alerta[i].nome_cb);
        
        printf("[UI] Verificando alerta %s: '%s'\n", checkboxes_alerta[i].descricao, comando);
        nextion_send_command(comando);
        vTaskDelay(pdMS_TO_TICKS(150)); // Delay maior
        
        // Variáveis para controle
        uint8_t valor = 0;
        bool valor_encontrado = false;
        
        // Lê todos os bytes disponíveis em um buffer
        uint8_t buffer[16] = {0};
        int idx = 0;
        uint32_t timeout = to_ms_since_boot(get_absolute_time()) + 300;
        
        while (to_ms_since_boot(get_absolute_time()) < timeout && uart_is_readable(UART_ID) && idx < 16) {
            buffer[idx] = uart_getc(UART_ID);
            printf("[UI] Byte %d: 0x%02X (%c)\n", idx, buffer[idx], 
                  (buffer[idx] >= 32 && buffer[idx] <= 126) ? buffer[idx] : '?');
            idx++;
        }
        
        // BUSCA O CÓDIGO 0x71 em QUALQUER POSIÇÃO DO BUFFER
        for (int j = 0; j < idx-4; j++) {
            if (buffer[j] == 0x71) {
                valor = buffer[j+1]; // O valor está no byte seguinte ao 0x71
                valor_encontrado = true;
                printf("[UI] ✅ Checkbox %s: valor=%d (encontrado na posição %d)\n", 
                      checkboxes_alerta[i].nome_cb, valor, j);
                break;
            }
        }
        
        // Se estiver marcado (valor = 1), adiciona à string
        if (valor == 1) {
            tem_alerta = true;
            
            // Se é o primeiro alerta, inicia com "ALERTA:"
            if (primeiro) {
                strcpy(alertas, "ALERTA: ");
                primeiro = false;
            } else {
                strcat(alertas, ", ");
            }
            
            // Adiciona o alerta
            strcat(alertas, checkboxes_alerta[i].descricao);
            printf("[UI] 🚨 Adicionado ALERTA CRÍTICO: %s\n", checkboxes_alerta[i].descricao);
        }
        
        // Limpa buffer antes da próxima leitura
        while (uart_is_readable(UART_ID)) {
            uart_getc(UART_ID);
        }
        
        // Pausa entre checkboxes
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    // Se algum alerta foi detectado, armazena
    if (tem_alerta) {
        strcpy(pacienteAtual.sintomas, alertas);
        printf("[UI] 🚨 ALERTAS CRÍTICOS capturados: %s\n", pacienteAtual.sintomas);
    } else {
        // Limpa sintomas se não há alertas
        strcpy(pacienteAtual.sintomas, "");
        printf("[UI] ℹ️ Nenhum alerta crítico selecionado\n");
    }
}