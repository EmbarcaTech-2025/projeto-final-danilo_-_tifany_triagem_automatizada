/**
 * @file task_ui.c
 * @brief Implementação da task de interface com display Nextion
 * 
 * Esta task gerencia toda a comunicação bidirecional com o display
 * Nextion via UART, processando comandos da FSM para atualizar a
 * interface e capturando eventos de toque do usuário.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#include "task_ui.h"

/**
 * @brief Task principal de gerenciamento da interface do usuário
 * @param pvParameters Parâmetros FreeRTOS (não utilizado)
 * 
 * Esta task executa em loop processando:
 * - Comandos da FSM para atualizar interface
 * - Eventos de toque do display Nextion
 * - Debounce de toques para evitar duplo clique
 * - Mapeamento de componentes por página
 */
void TaskUI(void *pvParameters)
{
    printf("[UI] Task iniciada\n");

    /* Aguarda estabilização do sistema */
    vTaskDelay(pdMS_TO_TICKS(200));

    /* Atualiza status inicial do display */
    atualizar_status_nextion();

    task_command_t cmd;

    /* Variáveis para sistema de debounce */
    static uint32_t last_touch_time = 0;
    static uint8_t last_component_id = 0xFF;
    const uint32_t DEBOUNCE_TIME_MS = 300;

    while (true)
    {
        /* 1. Processa comandos recebidos da FSM */
        if (xQueueReceive(xUICommandQueue, &cmd, 0) == pdTRUE)
        {
            char buffer[100];

            switch (cmd.cmd)
            {
            case UI_CMD_SHOW_PAGE:
                sprintf(buffer, "page %d", cmd.param1);
                nextion_send_command(buffer);
                printf("[UI] Página %d\n", cmd.param1);
                
                /* Confirmação especial para página de sintomas */
                if (cmd.param1 == 4) {
                    printf("[UI] ⚠️ Confirmando mudança para página de sintomas\n");
                    vTaskDelay(pdMS_TO_TICKS(50));
                    nextion_send_command("page 4");
                }
                
                /* Limpa campos quando retorna à tela inicial */
                if (cmd.param1 == 0)
                {
                    nextion_send_command("t_cpf_display.txt=\"\"");
                    nextion_send_command("va_cpf_len.val=0");
                    nextion_send_command("vis t_error_cpf,0");
                    nextion_send_command("tm_error.en=0");
                    nextion_send_command("t1.txt=\"\"");
                    nextion_send_command("t2.txt=\"\"");
                    nextion_send_command("t_conn.txt=\"\""); // Limpa campo de conexão
                    nextion_send_command("t_bottom.txt=\"\""); // Limpa campo de timer
                    printf("[UI] ✅ Campos limpos para novo paciente\n");
                }

                // FORÇA LIMPEZA NA PÁGINA DE CPF TAMBÉM
                if (cmd.param1 == 3)
                {
                    vTaskDelay(pdMS_TO_TICKS(100));
                    nextion_send_command("t_cpf_display.txt=\"\"");
                    nextion_send_command("va_cpf_len.val=0");
                    nextion_send_command("vis t_error_cpf,0");
                    nextion_send_command("tm_error.en=0");
                    printf("[UI] ✅ Campo CPF limpo na página 3\n");
                }
                break;

            case UI_CMD_UPDATE_TEXT:
                /* Se param1 == 0, usa t_bottom.txt (para contagem regressiva) */
                /* Senão usa t1.txt (comportamento padrão) */
                if (cmd.param1 == 0) {
                    sprintf(buffer, "t_bottom.txt=\"%.60s\"", cmd.text);
                } else {
                    sprintf(buffer, "t1.txt=\"%.60s\"", cmd.text);
                }
                nextion_send_command(buffer);
                break;

            case UI_CMD_UPDATE_PROGRESS:
                sprintf(buffer, "j0.val=%d", cmd.param1);
                nextion_send_command(buffer);
                break;

            default:
                break;
            }
        }

        /* 2. Escuta e processa eventos do display Nextion */
        if (uart_is_readable(UART_ID))
        {
            uint8_t header = uart_getc(UART_ID);

            /* Filtra bytes de lixo comum */
            if (header == 0xFF || header == 0x1A)
            {
                continue;
            }

            /* Filtra headers inválidos conhecidos */
            if (header == 0x05 || header == 0x03 || header == 0x01 || header == 0x02)
            {
                printf("[UI DEBUG] Header ignorado: 0x%02X (lixo)\n", header);
                continue;
            }

            printf("[UI DEBUG] Header recebido: 0x%02X\n", header);

            if (header == NEXTION_STARTUP_HEADER)
            {
                printf("[UI] Nextion inicializando...\n");
            }
            else if (header == NEXTION_READY_HEADER)
            {
                printf("[UI] Nextion pronto!\n");
                while (uart_is_readable(UART_ID) && uart_getc(UART_ID) == 0xFF)
                    ;
            }
            else if (header == NEXTION_TOUCH_EVENT_HEADER)
            {
                uint8_t page_id = uart_getc(UART_ID);
                uint8_t component_id = uart_getc(UART_ID);
                uint8_t event_type = uart_getc(UART_ID);

                /* Sistema de debounce para evitar duplo clique */
                uint32_t current_time = to_ms_since_boot(get_absolute_time());
                if ((current_time - last_touch_time) < DEBOUNCE_TIME_MS &&
                    component_id == last_component_id)
                {
                    printf("[UI DEBUG] DEBOUNCE: Toque ignorado (muito rápido)\n");
                    while (uart_is_readable(UART_ID) && uart_getc(UART_ID) == 0xFF)
                        ;
                    continue;
                }

                last_touch_time = current_time;
                last_component_id = component_id;

                printf("[UI] TOQUE: Página %d, Componente %d, Evento %d\n",
                       page_id, component_id, event_type);

                /* Aceita apenas eventos tipo 0 e 1 (Press e Release) */
                if (event_type != 0 && event_type != 1)
                {
                    printf("[UI DEBUG] Evento tipo %d ignorado\n", event_type);
                    while (uart_is_readable(UART_ID) && uart_getc(UART_ID) == 0xFF)
                        ;
                    continue;
                }

                /* Mapeia componentes baseado na estrutura de páginas */
                switch (page_id)
                {
                case 0: // Tela inicial
                    if (component_id == 7) {
                        printf("[UI] → Iniciando sistema\n");
                        xEventGroupSetBits(xEvents, EVT_UI_TOUCH_START);
                    }
                    break;

                case 1: // Tela de aviso (Page 1)
                    if (component_id == 2) { // Corrigido: era 3, agora é 2
                        printf("[UI] → Prosseguindo dos avisos\n");
                        xEventGroupSetBits(xEvents, EVT_UI_TOUCH_NEXT);
                    }
                    break;

                case 2: // Tela LGPD (Page 2)
                    if (component_id == 2) { // Corrigido: era 3, agora é 2 (SIM)
                        printf("[UI] → LGPD aceito (SIM)\n");
                        xEventGroupSetBits(xEvents, EVT_LGPD_ACCEPTED);
                    } else if (component_id == 3) { // Corrigido: era 4, agora é 3 (NÃO)
                        printf("[UI] → LGPD rejeitado (NÃO) - reiniciando sistema\n");
                        xEventGroupSetBits(xEvents, EVT_LGPD_REJECTED);
                    }
                    break;

                case 3: // Tela CPF
                    if (component_id == 4) {
                        printf("[UI] → Validando CPF\n");
                        
                        // Solicita o CPF atual
                        nextion_send_command("prints t_cpf_display.txt,0");
                        vTaskDelay(pdMS_TO_TICKS(100));

                        // Lê resposta da Nextion
                        char cpf_captured[20] = {0};
                        int cpf_idx = 0;

                        uint32_t timeout = to_ms_since_boot(get_absolute_time()) + 500;
                        while (to_ms_since_boot(get_absolute_time()) < timeout && uart_is_readable(UART_ID)) {
                            uint8_t byte = uart_getc(UART_ID);
                            
                            if (byte >= '0' && byte <= '9' && cpf_idx < 15) {
                                cpf_captured[cpf_idx++] = byte;
                            }
                        }

                        cpf_captured[cpf_idx] = '\0';
                        printf("[UI] CPF capturado: '%s' (len=%d)\n", cpf_captured, strlen(cpf_captured));

                        if (strlen(cpf_captured) == 11) {
                            strcpy(pacienteAtual.cpf, cpf_captured);
                            printf("[UI] ✅ CPF válido e armazenado: %s\n", pacienteAtual.cpf);
                            xEventGroupSetBits(xEvents, EVT_UI_TOUCH_NEXT);
                        } else {
                            printf("[UI] ❌ CPF inválido\n");
                            nextion_send_command("vis t_error_cpf,1");
                            nextion_send_command("tm_error.en=1");
                        }
                    }
                    break;

                case 4: // Tela de alertas críticos (nova)
                    if (component_id == 1) {
                        printf("[UI] → Capturando alertas críticos\n");
                        capturar_alertas();
                        xEventGroupSetBits(xEvents, EVT_UI_TOUCH_NEXT);
                    }
                    break;

                case 5: // Tela sintomas (mudou de 4 para 5)
                    if (component_id == 1) {
                        printf("[UI] → Capturando sintomas selecionados\n");
                        capturar_sintomas();
                        xEventGroupSetBits(xEvents, EVT_UI_TOUCH_NEXT);
                    }
                    break;

                case 8: // Tela final (mudou de 7 para 8)
                    if (component_id == 1) {
                        printf("[UI] → Finalizando triagem\n");
                        xEventGroupSetBits(xEvents, EVT_UI_TOUCH_FINISH);
                    }
                    break;

                default:
                    printf("[UI] → Página %d, Componente %d não mapeado\n", page_id, component_id);
                    break;
                }

                while (uart_is_readable(UART_ID) && uart_getc(UART_ID) == 0xFF)
                    ;
            }
            else
            {
                printf("[UI DEBUG] Header desconhecido: 0x%02X\n", header);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
