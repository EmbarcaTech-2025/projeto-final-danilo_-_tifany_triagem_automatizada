/**
 * @file wifi_client.c
 * @brief Implementação do cliente WiFi e comunicação HTTP
 * 
 * Este arquivo implementa as funcionalidades de conexão WiFi, sincronização NTP,
 * formatação de timestamps e comunicação HTTP com servidor para envio de dados
 * dos pacientes do sistema de triagem.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#include "wifi_client.h"
#include "nextion.h"

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "hardware/rtc.h"
#include "lwip/sockets.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"

#define TEST_TASK_PRIORITY (tskIDLE_PRIORITY + 1UL)

/* Sinalização de estado da conexão WiFi */
#define WIFI_CONNECTED_BIT (1 << 0)
static EventGroupHandle_t wifi_event_group;
static volatile bool g_connected = false;

/* Estrutura de estado para cliente NTP */
typedef struct NTP_T_
{
    ip_addr_t ntp_server_address;      /**< Endereço do servidor NTP */
    struct udp_pcb *ntp_pcb;           /**< PCB UDP para NTP */
    alarm_id_t ntp_timeout_alarm;      /**< Alarme para timeout NTP */
} NTP_T;

/* Referência externa para função do display Nextion */
extern void nextion_send_command(const char *cmd);

static void wifi_led_set(int on)
{
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, on ? 1 : 0);
}

char* get_timestamp() {
    static char timestamp[20];
    datetime_t t;
    
    rtc_get_datetime(&t);
    
    snprintf(timestamp, sizeof(timestamp), 
             "%02d/%02d/%04d %02d:%02d:%02d",
             t.day, t.month, t.year,
             t.hour, t.min, t.sec);
             
    return timestamp;
}

// Sincroniza RTC da Nextion com RTC do Pico
void sync_nextion_rtc(void) {
    datetime_t t;
    if (rtc_get_datetime(&t)) {
        char cmd_buffer[64];
        
        // Atualiza variáveis RTC da Nextion (rtc0-rtc5)
        sprintf(cmd_buffer, "rtc0=%d", t.year);
        nextion_send_command(cmd_buffer);
        
        sprintf(cmd_buffer, "rtc1=%d", t.month);
        nextion_send_command(cmd_buffer);
        
        sprintf(cmd_buffer, "rtc2=%d", t.day);
        nextion_send_command(cmd_buffer);
        
        sprintf(cmd_buffer, "rtc3=%d", t.hour);
        nextion_send_command(cmd_buffer);
        
        sprintf(cmd_buffer, "rtc4=%d", t.min);
        nextion_send_command(cmd_buffer);
        
        sprintf(cmd_buffer, "rtc5=%d", t.sec);
        nextion_send_command(cmd_buffer);
        
        printf("[RTC] ✅ Nextion sincronizada: %s\n", get_timestamp());
    }
}

// RTC simples como fallback
void set_simple_rtc(void) {
    printf("[RTC] Definindo horário padrão...\n");
    
    // Define horário padrão válido (25/08/2024 18:30:00)
    datetime_t default_dt = {
        .year = 2024,
        .month = 8,
        .day = 25,
        .dotw = 0, // Domingo
        .hour = 18,
        .min = 30,
        .sec = 0
    };
    
    rtc_init();
    vTaskDelay(pdMS_TO_TICKS(100));
    rtc_set_datetime(&default_dt);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    printf("[RTC] Horário padrão definido: %s\n", get_timestamp());
    sync_nextion_rtc();
}

// Função para setar o horário do RTC com o tempo ajustado
static void set_rtc_time_from_ntp(struct tm *time_info)
{
    datetime_t t = {
        .year = time_info->tm_year + 1900,
        .month = time_info->tm_mon + 1,
        .day = time_info->tm_mday,
        .dotw = time_info->tm_wday,
        .hour = time_info->tm_hour,
        .min = time_info->tm_min,
        .sec = time_info->tm_sec
    };

    rtc_init();
    vTaskDelay(pdMS_TO_TICKS(100));
    rtc_set_datetime(&t);
    vTaskDelay(pdMS_TO_TICKS(100));

    printf("[NTP] Horário RTC do Pico (GMT-3): %s\n", get_timestamp());
    
    // Sincroniza com a Nextion após definir o RTC
    sync_nextion_rtc();
}

// Chamado quando a resposta NTP é recebida
static void resultado_ntp(NTP_T *state, int status, time_t *result)
{
    if (status == 0 && result)
    {
        struct tm *utc = gmtime(result);
        printf("[NTP] Resposta NTP (GMT): %02d/%02d/%04d %02d:%02d:%02d\n", 
               utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
               utc->tm_hour, utc->tm_min, utc->tm_sec);

        // Converte o tempo UTC para o horário local (GMT-3 - Brasília)
        utc->tm_hour -= 3;
        if (utc->tm_hour < 0)
        {
            utc->tm_hour += 24;
            utc->tm_mday--;
        }

        // Configura o horário do RTC
        set_rtc_time_from_ntp(utc);
    }
    else
    {
        printf("[NTP] ⚠️  Falha na sincronização NTP, usando horário padrão\n");
        set_simple_rtc(); // Fallback para horário padrão
    }

    if (state->ntp_timeout_alarm > 0)
    {
        cancel_alarm(state->ntp_timeout_alarm);
        state->ntp_timeout_alarm = 0;
    }
    
    // LIBERA O STATE AQUI PARA EVITAR MEMORY LEAK
    if (state->ntp_pcb) {
        udp_remove(state->ntp_pcb);
    }
    free(state);
}

// Handler para timeout do NTP
static int64_t handler_ntp_timeout(alarm_id_t id, void *user_data)
{
    NTP_T *state = (NTP_T *)user_data;
    printf("[NTP] Timeout - usando horário padrão\n");
    resultado_ntp(state, -1, NULL);
    return 0;
}

// Faz uma requisição NTP
static void request_ntp(NTP_T *state)
{
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *)p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
    
    printf("[NTP] Pacote NTP enviado para %s\n", ipaddr_ntoa(&state->ntp_server_address));
}

// Dados do servidor NTP recebidos
static void ntp_recebido(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
    NTP_T *state = (NTP_T *)arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    printf("[NTP] Pacote recebido de %s:%d (len=%d)\n", ipaddr_ntoa(addr), port, p->tot_len);

    // Checa se a resposta é válida
    if (ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
        mode == 0x4 && stratum != 0)
    {
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        time_t epoch = seconds_since_1970;
        
        printf("[NTP] ✅ Pacote NTP válido recebido!\n");
        resultado_ntp(state, 0, &epoch);
    }
    else
    {
        printf("[NTP] ❌ Pacote NTP inválido (mode=%d, stratum=%d, len=%d)\n", mode, stratum, p->tot_len);
        resultado_ntp(state, -1, NULL);
    }
    pbuf_free(p);
}

// Inicializa o estado NTP
static NTP_T *ntp_init(void)
{
    NTP_T *state = (NTP_T *)calloc(1, sizeof(NTP_T));
    if (!state)
    {
        printf("[NTP] Falha em alocar estado\n");
        return NULL;
    }
    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state->ntp_pcb)
    {
        printf("[NTP] Falha ao criar pcb\n");
        free(state);
        return NULL;
    }
    udp_recv(state->ntp_pcb, ntp_recebido, state);
    return state;
}

// VERSÃO SIMPLIFICADA SEM DNS - USA IP DIRETO
void get_time_ntp(void)
{
    printf("[NTP] 🌐 Iniciando sincronização NTP...\n");
    
    NTP_T *state = ntp_init();
    if (!state) {
        printf("[NTP] ❌ Falha ao inicializar NTP, usando horário padrão\n");
        set_simple_rtc();
        return;
    }

    // USA IP FIXO DO SERVIDOR NTP BRASILEIRO (sem DNS)
    // 200.160.7.186 = ntp.br (servidor NTP do Brasil)
    ip_addr_t ntp_server_ip;
    IP4_ADDR(&ntp_server_ip, 200, 160, 7, 186);
    state->ntp_server_address = ntp_server_ip;
    
    printf("[NTP] Usando servidor NTP: %s (Brasil)\n", ipaddr_ntoa(&ntp_server_ip));

    // Seta timeout de 8 segundos
    state->ntp_timeout_alarm = add_alarm_in_ms(8000, handler_ntp_timeout, state, true);

    // Envia requisição NTP diretamente
    request_ntp(state);
    
    printf("[NTP] Requisição NTP enviada, aguardando resposta...\n");
}

// =================== WIFI FUNCTIONS ===================
void wifi_task(__unused void *params)
{
    if (cyw43_arch_init())
    {
        printf("failed to initialise\n");
        return;
    }

    cyw43_arch_enable_sta_mode();
    wifi_led_set(0);

    printf("Connecting to Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("failed to connect.\n");
        // Atualiza status no Nextion
        atualizar_status_nextion();
        
        // Em vez de exit(1), define horário padrão
        printf("[WiFi] WiFi falhou, usando horário padrão\n");
        set_simple_rtc();
    }
    else
    {
        const ip4_addr_t *ip = netif_ip4_addr(netif_default);
        printf("[WiFi] CONNECTED. IP: %s\n", ip4addr_ntoa(ip));
        g_connected = true;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        wifi_led_set(1);
        
        // Atualiza status no Nextion
        atualizar_status_nextion();
        
        // SINCRONIZA HORÁRIO VIA NTP ASSIM QUE CONECTA
        vTaskDelay(pdMS_TO_TICKS(2000)); // Aguarda estabilizar
        get_time_ntp();
        
        // Fallback se NTP demorar
        vTaskDelay(pdMS_TO_TICKS(10000)); // Aguarda 10s para NTP
        datetime_t check_time;
        if (rtc_get_datetime(&check_time) && check_time.year < 2024) {
            printf("[WiFi] NTP demorou, usando horário padrão como backup...\n");
            set_simple_rtc();
        }
    }

    // Loop principal - verifica e atualiza status periodicamente
    TickType_t last_check = xTaskGetTickCount();
    const TickType_t check_period = pdMS_TO_TICKS(30000); // 30 segundos
    
    while (true)
    {
        TickType_t now = xTaskGetTickCount();
        if ((now - last_check) >= check_period) {
            // Verifica status do WiFi periodicamente
            bool was_connected = g_connected;
            g_connected = (cyw43_tcpip_link_status(&cyw43_state, CYW43_ITF_STA) == CYW43_LINK_UP);
            
            // Se mudou de estado ou tempo de verificação, atualiza display
            if (was_connected != g_connected || (now - last_check) >= check_period) {
                atualizar_status_nextion();
                last_check = now;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    cyw43_arch_deinit();
}

void wifi_start(void)
{
    wifi_event_group = xEventGroupCreate();
    if (!wifi_event_group) {
        printf("ERRO: Falha ao criar wifi_event_group!\n");
        return;
    }
    
    TaskHandle_t task;
    xTaskCreate(wifi_task, "WiFi", 4096, NULL, TEST_TASK_PRIORITY, &task);
}

// Função para formatar CPF no padrão brasileiro
static void formatar_cpf(const char *cpf_raw, char *cpf_formatado) {
    if (strlen(cpf_raw) != 11) {
        // CPF inválido, apenas copia sem formatar
        strcpy(cpf_formatado, cpf_raw);
        return;
    }
    
    // Formato: XXX.XXX.XXX-XX
    sprintf(cpf_formatado, "%c%c%c.%c%c%c.%c%c%c-%c%c",
            cpf_raw[0], cpf_raw[1], cpf_raw[2],
            cpf_raw[3], cpf_raw[4], cpf_raw[5],
            cpf_raw[6], cpf_raw[7], cpf_raw[8],
            cpf_raw[9], cpf_raw[10]);
}

// ===== Resto das funções HTTP e WiFi (iguais) =====
int enviar_dados_servidor(const paciente_t *p) {
    if (!p || !g_connected) {
        printf("[HTTP] Erro: dados inválidos ou WiFi desconectado\n");
        return -1;
    }

    printf("[HTTP] Iniciando envio HTTP real...\n");

    // Formata o CPF para o padrão brasileiro
    char cpf_formatado[20];
    formatar_cpf(p->cpf, cpf_formatado);
    printf("[HTTP] CPF formatado: %s\n", cpf_formatado);

    static char json[768];   // Reduzido de 1024 para 768
    static char req[1024];   // Reduzido de 1500 para 1024
    
    int jn = snprintf(json, sizeof(json),
        "{\"nome\":\"%.20s\",\"cpf\":\"%.15s\",\"sintomas\":\"%.400s\","  // Reduzido para 400 caracteres de sintomas
        "\"bpm\":%d,\"spo2\":%d,\"temperatura\":%.1f,"
        "\"pressao_sys\":%.0f,\"pressao_dia\":%.0f,\"distancia\":%d}",
        p->nome, cpf_formatado, p->sintomas,
        p->bpm, p->spo2, p->temperatura,
        p->pressao_sys, p->pressao_dia, p->distancia_mm);
    
    if (jn <= 0 || jn >= (int)sizeof(json)) {
        printf("[HTTP] ERRO: JSON muito grande (tamanho=%d)\n", jn);
        return -2;
    }

    printf("[HTTP] JSON: %s\n", json);

    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        printf("[HTTP] ERRO: socket() falhou: %d\n", s);
        return -3;
    }

    printf("[HTTP] Socket criado: %d\n", s);

    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    if (setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        printf("[HTTP] Erro ao definir timeout de envio\n");
    }
    if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        printf("[HTTP] Erro ao definir timeout de recebimento\n");
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);

    printf("[HTTP] Conectando para %s:%d...\n", SERVER_IP, SERVER_PORT);

    if (connect(s, (struct sockaddr*)&server, sizeof(server)) != 0) {
        printf("[HTTP] ERRO: Falha ao conectar no servidor\n");
        close(s);
        return -4;
    }

    printf("[HTTP] ✅ Conectado com sucesso!\n");

    int rn = snprintf(req, sizeof(req),
        "POST /api/paciente HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        SERVER_IP, SERVER_PORT, jn, json);
    
    if (rn <= 0 || rn >= (int)sizeof(req)) {
        printf("[HTTP] ERRO: Falha ao criar requisição (tamanho=%d)\n", rn);
        close(s);
        return -5;
    }

    printf("[HTTP] Enviando %d bytes...\n", rn);

    int bytes_sent = send(s, req, rn, 0);
    if (bytes_sent < 0) {
        printf("[HTTP] ERRO: send() falhou: %d\n", bytes_sent);
        close(s);
        return -6;
    }

    printf("[HTTP] ✅ Enviados %d de %d bytes\n", bytes_sent, rn);

    char response[200];
    int bytes_received = recv(s, response, sizeof(response) - 1, 0);
    if (bytes_received > 0) {
        response[bytes_received] = '\0';
        printf("[HTTP] Resposta: %.100s\n", response);
        
        if (strstr(response, "200 OK") != NULL) {
            printf("[HTTP] ✅ Servidor confirmou recebimento!\n");
        } else {
            printf("[HTTP] ⚠️  Resposta não indica sucesso\n");
        }
    } else if (bytes_received == 0) {
        printf("[HTTP] Conexão fechada pelo servidor (normal)\n");
    } else {
        printf("[HTTP] Timeout ou erro ao receber resposta\n");
    }

    close(s);
    printf("📡 [HTTP] ✅ Envio HTTP concluído com sucesso!\n");
    
    return 0;
}

bool wifi_wait_connected(uint32_t timeout_ms) {
    if (!wifi_event_group) {
        printf("ERRO: Event group do WiFi não foi criado!\n");
        return false;
    }
    
    TickType_t timeout_ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    
    printf("Aguardando WiFi conectar (timeout: %lu ms)...\n", timeout_ms);
    
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdFALSE,
        timeout_ticks
    );
    
    bool connected = (bits & WIFI_CONNECTED_BIT) != 0;
    printf("WiFi wait resultado: %s\n", connected ? "CONECTADO" : "TIMEOUT");
    
    return connected;
}

bool wifi_is_connected(void) {
    return g_connected;
}

// Verifica se o servidor está online (tentando conectar)
bool testar_servidor(void) {
    if (!g_connected) {
        return false;
    }
    
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
        return false;
    }
    
    // Configurar timeout curto (1 segundo)
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    int result = connect(s, (struct sockaddr*)&server, sizeof(server));
    close(s);
    
    return (result == 0);
}

// Atualiza o texto de status na tela Nextion
void atualizar_status_nextion(void) {
    char buffer[64];
    bool wifi_ok = g_connected;
    bool servidor_ok = testar_servidor();
    
    // Formata o texto de status
    snprintf(buffer, sizeof(buffer), "WiFi: %s | Servidor: %s", 
             wifi_ok ? "Online" : "Offline", 
             servidor_ok ? "Online" : "Offline");
    
    // Atualiza o texto na tela usando o novo campo t_conn
    char comando[128];
    snprintf(comando, sizeof(comando), "t_conn.txt=\"%s\"", buffer);
    nextion_send_command(comando);
    
    printf("[WiFi] Status atualizado: %s\n", buffer);
}