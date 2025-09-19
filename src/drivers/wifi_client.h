/**
 * @file wifi_client.h
 * @brief Driver para conectividade WiFi e comunicação com servidor
 * 
 * Este módulo gerencia a conexão WiFi, sincronização NTP para obtenção de horário
 * e comunicação HTTP com servidor remoto para envio de dados dos pacientes.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "pico/stdlib.h"

/* Configuração da rede WiFi */
#define WIFI_SSID "ssid"          /**< SSID da rede WiFi */
#define WIFI_PASS "senha"       /**< Senha da rede WiFi */

/* Configuração do servidor HTTP */
#define SERVER_IP      "0.0.0.0" /**< Endereço IP do servidor Flask */
#define SERVER_PORT    5000            /**< Porta do servidor HTTP */

/* Configuração do cliente NTP */
#define NTP_SERVER "pool.ntp.org"      /**< Servidor NTP para sincronização */
#define NTP_MSG_LEN 48                 /**< Tamanho da mensagem NTP */
#define NTP_PORT 123                   /**< Porta padrão NTP */
#define NTP_DELTA 2208988800           /**< Delta entre épocas NTP e Unix */
#define NTP_TEST_TIME (30 * 1000)      /**< Timeout para teste NTP */
#define NTP_RESEND_TIME (10 * 1000)    /**< Intervalo para reenvio NTP */

/* Estrutura de dados do paciente */
typedef struct {
    char nome[50];                     /**< Nome/identificação do paciente */
    char cpf[20];                      /**< CPF do paciente */
    char sintomas[400];                /**< String com sintomas selecionados */
    
    float temperatura;                 /**< Temperatura corporal em °C */
    int   bpm;                         /**< Batimentos cardíacos por minuto */
    int   spo2;                        /**< Saturação de oxigênio (%) */
    float pressao_sys;                 /**< Pressão sistólica em mmHg */
    float pressao_dia;                 /**< Pressão diastólica em mmHg */
    int   distancia_mm;                /**< Distância do sensor em mm */
    
    bool  valid;                       /**< Flag de validade dos dados */
} paciente_t;

/* Declarações das funções da API WiFi */

/**
 * @brief Inicia task de conexão WiFi
 * 
 * Cria uma task que tenta conectar ao WiFi repetidamente até obter sucesso.
 */
void wifi_start(void);

/**
 * @brief Aguarda conexão WiFi com timeout
 * @param timeout_ms Timeout em milissegundos (0 = espera infinita)
 * @return true se conectado, false se timeout expirado
 */
bool wifi_wait_connected(uint32_t timeout_ms);

/**
 * @brief Verifica se WiFi está conectado
 * @return true se conectado, false caso contrário
 */
bool wifi_is_connected(void);

/**
 * @brief Envia dados do paciente via HTTP POST
 * @param p Ponteiro para estrutura de dados do paciente
 * @return 0 em sucesso, código negativo em caso de erro
 */
int enviar_dados_servidor(const paciente_t *p);

/**
 * @brief Testa conectividade com o servidor
 * @return true se servidor acessível, false caso contrário
 */
bool testar_servidor(void);

/**
 * @brief Atualiza status da conexão no display Nextion
 */
void atualizar_status_nextion(void);
