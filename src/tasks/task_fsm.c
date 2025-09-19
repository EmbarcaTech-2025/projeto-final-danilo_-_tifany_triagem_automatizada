/**
 * @file task_fsm.c
 * @brief Implementação da Máquina de Estados Finitos principal
 * 
 * Este arquivo implementa o "cérebro" do sistema de triagem, coordenando
 * todo o fluxo desde a tela inicial até o envio de dados. A FSM gerencia
 * estados, timeouts, coleta de dados e integração entre todas as tasks.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#include "task_fsm.h"

/* Variáveis globais do sistema */
uint32_t numero_atendimento = 1000001;    /**< Contador sequencial de atendimentos */
estado_t estado_atual = ESTADO_INICIAL;   /**< Estado atual da máquina de estados */
paciente_t pacienteAtual;                 /**< Dados do paciente sendo atendido */

/* Recursos FreeRTOS */
QueueHandle_t xDataQueue;      /**< Fila para envio de dados ao servidor */
SemaphoreHandle_t xI2CMutex;   /**< Mutex para controle de acesso I2C */

/**
 * @brief Task principal da Máquina de Estados Finitos
 * @param pvParameters Parâmetros FreeRTOS (não utilizado)
 * 
 * Esta função implementa o fluxo completo da triagem:
 * - Tela inicial e avisos
 * - Termo LGPD
 * - Cadastro (CPF)
 * - Alertas críticos
 * - Sintomas
 * - Medições (PPG e temperatura)
 * - Resultados e envio de dados
 */
void TaskFSM(void *pvParameters)
{
    task_command_t cmd;
    EventBits_t events;

    printf("[FSM] Cérebro iniciado - Pico controlando tudo!\n");

    while (true)
    {
        /* Reseta dados do paciente para nova sessão */
        pacienteAtual = (paciente_t){0};
        
        /* Gera número de atendimento sequencial de 7 dígitos */
        sprintf(pacienteAtual.nome, "Atendimento #%07lu", (unsigned long)numero_atendimento);
        printf("[FSM] 🎫 Novo atendimento gerado: %s\n", pacienteAtual.nome);

        /* ===== ESTADO INICIAL ===== */
        estado_atual = ESTADO_INICIAL;
        printf("[FSM] Sistema pronto - aguardando toque na tela...\n");

        /* Exibe tela inicial (Page 0) */
        cmd = (task_command_t){
            .cmd = UI_CMD_SHOW_PAGE,
            .param1 = 0 // página inicial
        };
        xQueueSend(xUICommandQueue, &cmd, 0);

        /* Aguarda toque do usuário para iniciar */
        xEventGroupClearBits(xEvents, EVT_UI_TOUCH_START);
        xEventGroupWaitBits(xEvents, EVT_UI_TOUCH_START, pdTRUE, pdFALSE, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(300));

        /* ===== ESTADO AVISO ===== */
        estado_atual = ESTADO_AVISO;
        printf("[FSM] Mostrando avisos importantes...\n");

        /* Reproduz áudio de aviso */
        cmd = (task_command_t){
            .cmd = AUDIO_CMD_PLAY,
            .param1 = 1};
        strcpy(cmd.filename, "aviso.wav");
        xQueueSend(xAudioCommandQueue, &cmd, 0);

        vTaskDelay(pdMS_TO_TICKS(500));

        // Muda para tela de aviso (Page 1)
        cmd.cmd = UI_CMD_SHOW_PAGE;
        cmd.param1 = 1;
        xQueueSend(xUICommandQueue, &cmd, 0);

        // TIMEOUT DE 30 SEGUNDOS COM CONTAGEM REGRESSIVA
        printf("[FSM] Aguardando usuário ler avisos (30s timeout)...\n");
        xEventGroupClearBits(xEvents, EVT_UI_TOUCH_NEXT);
        
        // Loop de contagem regressiva
        for (int countdown = 30; countdown > 0; countdown--) {
            // Atualiza display com contagem regressiva
            sprintf(cmd.text, "Tempo restante: %ds", countdown);
            cmd.cmd = UI_CMD_UPDATE_TEXT;
            cmd.param1 = 0; // Usar t_bottom.txt
            xQueueSend(xUICommandQueue, &cmd, 0);
            
            // Aguarda 1 segundo ou evento
            EventBits_t result = xEventGroupWaitBits(xEvents, EVT_UI_TOUCH_NEXT, pdTRUE, pdFALSE, pdMS_TO_TICKS(1000));
            
            if (result & EVT_UI_TOUCH_NEXT) {
                printf("[FSM] Usuário confirmou avisos\n");
                goto aviso_confirmado;
            }
        }
        
        // Se chegou aqui, timeout ocorreu
        printf("[FSM] Timeout na tela de avisos - reiniciando...\n");
        cmd.cmd = UI_CMD_UPDATE_TEXT;
        strcpy(cmd.text, "Tempo esgotado! Reiniciando...");
        cmd.param1 = 0;
        xQueueSend(xUICommandQueue, &cmd, 0);
        vTaskDelay(pdMS_TO_TICKS(2000));
        continue; // Volta para o início do loop
        
        aviso_confirmado:
        vTaskDelay(pdMS_TO_TICKS(300));

        // ===== ESTADO LGPD =====
        estado_atual = ESTADO_LGPD;
        printf("[FSM] Solicitando aceite dos termos LGPD...\n");

        // Toca áudio LGPD
        cmd = (task_command_t){
            .cmd = AUDIO_CMD_PLAY,
            .param1 = 2};
        strcpy(cmd.filename, "lgpd.wav");
        xQueueSend(xAudioCommandQueue, &cmd, 0);

        vTaskDelay(pdMS_TO_TICKS(500));

        // Muda para tela LGPD (Page 2)
        cmd.cmd = UI_CMD_SHOW_PAGE;
        cmd.param1 = 2;
        xQueueSend(xUICommandQueue, &cmd, 0);

        // TIMEOUT DE 30 SEGUNDOS COM CONTAGEM REGRESSIVA
        printf("[FSM] Aguardando aceite/rejeição LGPD (30s timeout)...\n");
        xEventGroupClearBits(xEvents, EVT_LGPD_ACCEPTED | EVT_LGPD_REJECTED);
        
        // Loop de contagem regressiva
        bool lgpd_accepted = false;
        bool lgpd_rejected = false;
        
        for (int countdown = 30; countdown > 0; countdown--) {
            // Atualiza display com contagem regressiva
            sprintf(cmd.text, "Aceita os termos? Tempo: %ds", countdown);
            cmd.cmd = UI_CMD_UPDATE_TEXT;
            cmd.param1 = 0; // Usar t_bottom.txt
            xQueueSend(xUICommandQueue, &cmd, 0);
            
            // Aguarda 1 segundo ou evento
            EventBits_t result = xEventGroupWaitBits(xEvents, 
                                                   EVT_LGPD_ACCEPTED | EVT_LGPD_REJECTED, 
                                                   pdTRUE, pdFALSE, 
                                                   pdMS_TO_TICKS(1000));
            
            if (result & EVT_LGPD_REJECTED) {
                printf("[FSM] Termos LGPD rejeitados - reiniciando sistema...\n");
                cmd.cmd = UI_CMD_UPDATE_TEXT;
                strcpy(cmd.text, "Termos rejeitados. Reiniciando...");
                cmd.param1 = 0;
                xQueueSend(xUICommandQueue, &cmd, 0);
                vTaskDelay(pdMS_TO_TICKS(2000));
                lgpd_rejected = true;
                break; // Sai do loop de countdown
            } else if (result & EVT_LGPD_ACCEPTED) {
                printf("[FSM] Termos LGPD aceitos - prosseguindo...\n");
                lgpd_accepted = true;
                break; // Sai do loop de countdown
            }
        }
        
        // Se LGPD foi rejeitado, reinicia o sistema completo
        if (lgpd_rejected) {
            continue; // Volta para o início do loop principal (reinicia tudo)
        }
        
        // Se timeout sem resposta, também reinicia
        if (!lgpd_accepted && !lgpd_rejected) {
            printf("[FSM] Timeout na tela de LGPD - reiniciando...\n");
            cmd.cmd = UI_CMD_UPDATE_TEXT;
            strcpy(cmd.text, "Tempo esgotado! Reiniciando...");
            cmd.param1 = 0;
            xQueueSend(xUICommandQueue, &cmd, 0);
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue; // Volta para o início do loop principal
        }

        // Se chegou aqui, LGPD foi aceito - continua o fluxo normal
        vTaskDelay(pdMS_TO_TICKS(300));

        // ===== ESTADO CADASTRO =====
        estado_atual = ESTADO_CADASTRO;
        printf("[FSM] Iniciando cadastro de CPF...\n");

        // Toca áudio de CPF
        cmd = (task_command_t){
            .cmd = AUDIO_CMD_PLAY,
            .param1 = 3};
        strcpy(cmd.filename, "cpf.wav");
        xQueueSend(xAudioCommandQueue, &cmd, 0);

        vTaskDelay(pdMS_TO_TICKS(500));

        // Muda para tela de cadastro (Page 3)
        cmd.cmd = UI_CMD_SHOW_PAGE;
        cmd.param1 = 3;
        xQueueSend(xUICommandQueue, &cmd, 0);

        // AGUARDA USUÁRIO PREENCHER CPF E CLICAR "AVANÇAR"
        printf("[FSM] Aguardando usuário preencher CPF e clicar 'Avançar'...\n");
        xEventGroupClearBits(xEvents, EVT_UI_TOUCH_NEXT);
        xEventGroupWaitBits(xEvents, EVT_UI_TOUCH_NEXT, pdTRUE, pdFALSE, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(300));

        // ===== ESTADO ALERTAS (NOVA TELA) =====
        estado_atual = ESTADO_ALERTAS;
        printf("[FSM] Verificando sintomas críticos de alerta...\n");

        // Toca áudio de alerta
        cmd = (task_command_t){
            .cmd = AUDIO_CMD_PLAY,
            .param1 = 8}; // Novo ID para alerta.wav
        strcpy(cmd.filename, "alerta.wav");
        xQueueSend(xAudioCommandQueue, &cmd, 0);

        vTaskDelay(pdMS_TO_TICKS(500));

        // Muda para tela de alertas (Page 4)
        cmd.cmd = UI_CMD_SHOW_PAGE;
        cmd.param1 = 4;
        xQueueSend(xUICommandQueue, &cmd, 0);

        // --- LIMPA CHECKBOXES DE ALERTAS ANTES DE MOSTRAR A TELA ---
        nextion_send_command("cb_dor.val=0");
        nextion_send_command("cb_neuro.val=0");
        nextion_send_command("cb_ar.val=0");
        nextion_send_command("cb_peito.val=0");
        nextion_send_command("cb_sang.val=0");

        // AGUARDA USUÁRIO SELECIONAR ALERTAS E CLICAR "AVANÇAR"
        printf("[FSM] Aguardando usuário verificar alertas críticos e clicar 'Avançar'...\n");
        xEventGroupClearBits(xEvents, EVT_UI_TOUCH_NEXT);
        xEventGroupWaitBits(xEvents, EVT_UI_TOUCH_NEXT, pdTRUE, pdFALSE, portMAX_DELAY);

        // Verifica se algum alerta crítico foi marcado
        bool tem_alerta_critico = (strlen(pacienteAtual.sintomas) > 0 && strstr(pacienteAtual.sintomas, "ALERTA:") != NULL);
        
        if (tem_alerta_critico) {
            printf("[FSM] ⚠️ ALERTA CRÍTICO detectado: %s\n", pacienteAtual.sintomas);
            printf("[FSM] Pulando medições - indo direto para resultados de urgência\n");
            
            // Define valores padrão para medições (não realizadas devido à urgência)
            pacienteAtual.bpm = 0;
            pacienteAtual.spo2 = 0;
            pacienteAtual.temperatura = 0.0f;
            pacienteAtual.pressao_sys = 0;
            pacienteAtual.pressao_dia = 0;
            pacienteAtual.distancia_mm = 0;
            pacienteAtual.valid = true; // Marcar como válido mesmo sem medições
            
            // Vai direto para resultados
            goto mostrar_resultados_urgencia;
        }

        printf("[FSM] Nenhum alerta crítico - prosseguindo para sintomas normais\n");
        vTaskDelay(pdMS_TO_TICKS(300));

        // ===== ESTADO SINTOMAS =====
        estado_atual = ESTADO_SINTOMAS;
        printf("[FSM] Coletando sintomas...\n");

        // Toca áudio de sintomas
        cmd = (task_command_t){
            .cmd = AUDIO_CMD_PLAY,
            .param1 = 4};
        strcpy(cmd.filename, "sintomas.wav");
        xQueueSend(xAudioCommandQueue, &cmd, 0);

        vTaskDelay(pdMS_TO_TICKS(500));

        // Muda para tela de sintomas (Page 5 - mudou!)
        cmd.cmd = UI_CMD_SHOW_PAGE;
        cmd.param1 = 5;
        xQueueSend(xUICommandQueue, &cmd, 0);

        printf("[FSM] Enviando comando direto para página de sintomas...\n");
        vTaskDelay(pdMS_TO_TICKS(100));
        nextion_send_command("page 5");

        // --- LIMPA CHECKBOXES ANTES DE MOSTRAR A TELA ---
        nextion_send_command("cb_febre.val=0");
        nextion_send_command("cb_fraq.val=0");
        nextion_send_command("cb_calaf.val=0");
        nextion_send_command("cb_cabeca.val=0");
        nextion_send_command("cb_tont.val=0");
        nextion_send_command("cb_garg.val=0");
        nextion_send_command("cb_gripe.val=0");
        nextion_send_command("cb_tosse.val=0");
        nextion_send_command("cb_abdom.val=0");
        nextion_send_command("cb_vomito.val=0");
        nextion_send_command("cb_diarreia.val=0");
        nextion_send_command("cb_azia.val=0");
        nextion_send_command("cb_corpo.val=0");
        nextion_send_command("cb_ferimento.val=0");
        nextion_send_command("cb_urina.val=0");
        nextion_send_command("cb_gineco.val=0");

        // AGUARDA USUÁRIO SELECIONAR SINTOMAS E CLICAR "AVANÇAR"
        printf("[FSM] Aguardando usuário selecionar sintomas e clicar 'Avançar'...\n");
        xEventGroupClearBits(xEvents, EVT_UI_TOUCH_NEXT);
        xEventGroupWaitBits(xEvents, EVT_UI_TOUCH_NEXT, pdTRUE, pdFALSE, portMAX_DELAY);

        printf("[FSM] Sintomas selecionados: %s\n", pacienteAtual.sintomas);
        vTaskDelay(pdMS_TO_TICKS(300));

        // ===== ESTADO PPG =====
        estado_atual = ESTADO_PPG;
        printf("[FSM] Iniciando medição cardíaca...\n");

        // Toca áudio PPG
        cmd = (task_command_t){
            .cmd = AUDIO_CMD_PLAY,
            .param1 = 5};
        strcpy(cmd.filename, "oximetro.wav");
        xQueueSend(xAudioCommandQueue, &cmd, 0);

        vTaskDelay(pdMS_TO_TICKS(500));

        // Muda para tela PPG (Page 6 - mudou!)
        cmd.cmd = UI_CMD_SHOW_PAGE;
        cmd.param1 = 6;
        xQueueSend(xUICommandQueue, &cmd, 0);

        // Espera task PPG completar
        xEventGroupClearBits(xEvents, EVT_PPG_READY);
        xEventGroupWaitBits(xEvents, EVT_PPG_READY, pdTRUE, pdFALSE, portMAX_DELAY);
        printf("[FSM] Medição cardíaca concluída: BPM=%d, SpO2=%d%%\n",
               pacienteAtual.bpm, pacienteAtual.spo2);

        vTaskDelay(pdMS_TO_TICKS(500));

        // ===== ESTADO TEMPERATURA =====
        estado_atual = ESTADO_TEMP;
        printf("[FSM] Iniciando medição de temperatura...\n");

        // Toca áudio temperatura
        cmd = (task_command_t){
            .cmd = AUDIO_CMD_PLAY,
            .param1 = 6};
        strcpy(cmd.filename, "temperatura.wav");
        xQueueSend(xAudioCommandQueue, &cmd, 0);

        vTaskDelay(pdMS_TO_TICKS(500));

        // Muda para tela temperatura (Page 7 - mudou!)
        cmd.cmd = UI_CMD_SHOW_PAGE;
        cmd.param1 = 7;
        xQueueSend(xUICommandQueue, &cmd, 0);

        // Espera task temperatura completar
        xEventGroupClearBits(xEvents, EVT_TEMP_READY);
        xEventGroupWaitBits(xEvents, EVT_TEMP_READY, pdTRUE, pdFALSE, portMAX_DELAY);
        printf("[FSM] Medição temperatura concluída: %.1f°C\n", pacienteAtual.temperatura);

        vTaskDelay(pdMS_TO_TICKS(500));

        mostrar_resultados_urgencia:
        // ===== ESTADO RESULTADOS =====
        estado_atual = ESTADO_RESULTADOS;
        printf("[FSM] Exibindo resultados...\n");

        // Toca áudio final
        cmd = (task_command_t){
            .cmd = AUDIO_CMD_PLAY,
            .param1 = 7};
        strcpy(cmd.filename, "final.wav");
        xQueueSend(xAudioCommandQueue, &cmd, 0);

        vTaskDelay(pdMS_TO_TICKS(500));

        // Muda para tela de resultados (Page 8 - mudou!)
        cmd.cmd = UI_CMD_SHOW_PAGE;
        cmd.param1 = 8;
        xQueueSend(xUICommandQueue, &cmd, 0);

        // Aguarda um pouco para a tela carregar antes de atualizar
        vTaskDelay(pdMS_TO_TICKS(100));

        // Atualiza apenas o número no campo t_num.txt (sem texto "Atendimento #")
        char num_cmd[32];
        sprintf(num_cmd, "t_num.txt=\"%07lu\"", (unsigned long)numero_atendimento);
        nextion_send_command(num_cmd);
        printf("[FSM] 📋 Número de atendimento exibido no t_num: %07lu\n", (unsigned long)numero_atendimento);

        sprintf(cmd.text, "CPF: %s | BPM: %d | SpO2: %d%% | Temp: %.1f°C | PA: %d/%d",
                pacienteAtual.cpf, pacienteAtual.bpm, pacienteAtual.spo2, pacienteAtual.temperatura,
                (int)pacienteAtual.pressao_sys, (int)pacienteAtual.pressao_dia);
        cmd.cmd = UI_CMD_UPDATE_TEXT;
        cmd.param1 = 1; // Usar campo normal para resultados
        xQueueSend(xUICommandQueue, &cmd, 0);

        // NOVO: Salvar dados no log JSON antes de enviar para servidor
        printf("[FSM] Salvando dados no log JSON...\n");
        if (save_patient_to_json_log(&pacienteAtual)) {
            printf("[FSM] ✅ Dados salvos no log JSON com sucesso\n");
        } else {
            printf("[FSM] ⚠️ Falha ao salvar no log JSON\n");
        }

        // Envia dados para servidor
        printf("[FSM] Enviando dados para servidor...\n");
        enviar_dados_servidor(&pacienteAtual);
        xQueueSend(xDataQueue, &pacienteAtual, 0);

        // AGUARDA USUÁRIO CLICAR "FINALIZAR" COM TIMEOUT E CONTAGEM REGRESSIVA
        printf("[FSM] Aguardando usuário clicar 'Finalizar' (60s timeout)...\n");
        xEventGroupClearBits(xEvents, EVT_UI_TOUCH_FINISH | EVT_UI_TOUCH_NEXT);
        
        bool user_clicked = false;
        for (int countdown = 60; countdown > 0 && !user_clicked; countdown--) {
            // Atualiza contagem regressiva na tela final
            sprintf(cmd.text, "Reinicio automatico em: %ds", countdown);
            cmd.cmd = UI_CMD_UPDATE_TEXT;
            cmd.param1 = 0; // Usar t_bottom.txt
            xQueueSend(xUICommandQueue, &cmd, 0);
            
            // Aguarda 1 segundo ou evento do usuário
            EventBits_t result = xEventGroupWaitBits(xEvents,
                                                   EVT_UI_TOUCH_FINISH | EVT_UI_TOUCH_NEXT,
                                                   pdTRUE, pdFALSE,
                                                   pdMS_TO_TICKS(1000));
            
            if (result & (EVT_UI_TOUCH_FINISH | EVT_UI_TOUCH_NEXT)) {
                printf("[FSM] Usuário confirmou finalização\n");
                user_clicked = true;
                break;
            }
        }
        
        if (!user_clicked) {
            printf("[FSM] Timeout de 60s - finalizando automaticamente\n");
        }

        printf("[FSM] Triagem finalizada - reiniciando sistema...\n");
        cmd.cmd = UI_CMD_UPDATE_TEXT;
        strcpy(cmd.text, "Reiniciando...");
        cmd.param1 = 0;
        xQueueSend(xUICommandQueue, &cmd, 0);
        vTaskDelay(pdMS_TO_TICKS(2000));
        
        // Incrementa o número de atendimento para o próximo paciente
        numero_atendimento++;
    }
}
