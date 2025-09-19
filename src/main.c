/**
 * @file main.c
 * @brief Programa principal do Sistema de Triagem Embarcatech
 * 
 * Este arquivo contém a função main() e a inicialização completa do sistema
 * de triagem baseado em FreeRTOS. Configura todos os periféricos, inicializa
 * sensores, sistema de arquivos SD e cria todas as tasks necessárias para
 * operação do sistema.
 * 
 * O sistema integra:
 * - Display Nextion para interface do usuário
 * - Sensores I2C (temperatura, oximetria, distância)
 * - Sistema de áudio I2S para reprodução WAV
 * - Conectividade WiFi para envio de dados
 * - Cartão SD para armazenamento de logs
 * - FreeRTOS para multitarefa em tempo real
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#include "pico/stdlib.h"
#include "pico/audio_i2s.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* Sistema de arquivos FAT para cartão SD */
#include "f_util.h"
#include "ff.h"

/* RTC usando biblioteca padrão do Pico */
#include "pico/util/datetime.h"
#include "hardware/rtc.h"
#include "lib/pico_sdcard/include/my_rtc.h"

/* Drivers do sistema */
#include "drivers/wifi_client.h"
#include "drivers/nextion.h"
#include "drivers/audio.h"

/* Tasks da aplicação */
#include "tasks/task_fsm.h"
#include "tasks/task_ui.h"
#include "tasks/task_audio.h"
#include "tasks/task_ppg.h"
#include "tasks/task_temp_dist.h"
#include "tasks/task_server.h"
#include "tasks/task_wifi_check.h"
#include "tasks/task_hooks.h"

/* Bibliotecas dos sensores */
#include "lib/sensor/max3010x/max3010x.h"
#include "lib/sensor/max3010x/spo2_algorithm.h"
#include "lib/sensor/max3010x/ppg_blood_pressure.h"
#include "lib/sensor/max3010x/heart_rate.h"
#include "lib/sensor/mlx90614/mlx90614.h"
#include "lib/sensor/vl53l0x/vl53l0x.h"

/* Sistema de logging */
#include "utils/json_logger.h"

/* Configurações do sistema */
#define SIMULACAO 0                 /**< 1 = simulação, 0 = sensores reais */

/* Configuração dos pinos I2C compartilhados */
#define I2C_PORT i2c1              /**< Porta I2C utilizada */
#define I2C_SDA_PIN 2              /**< Pino GPIO para SDA */
#define I2C_SCL_PIN 3              /**< Pino GPIO para SCL */

/* Configuração da comunicação UART com Nextion */
#define UART_ID uart0              /**< Instância UART */
#define BAUD_RATE 9600             /**< Taxa de transmissão */
#define UART_TX_PIN 0              /**< Pino GPIO TX */
#define UART_RX_PIN 1              /**< Pino GPIO RX */

/* Definições do protocolo Nextion */
#define NEXTION_STARTUP_HEADER 0x00      /**< Header de inicialização */
#define NEXTION_READY_HEADER 0x88        /**< Header de display pronto */
#define NEXTION_TOUCH_EVENT_HEADER 0x65  /**< Header de evento de toque */
#define NEXTION_END_OF_MESSAGE 0xFF      /**< Marcador fim de mensagem */

/* Sistema de eventos para comunicação entre tasks */
EventGroupHandle_t xEvents;  
#define EVT_PPG_READY (1 << 0)           /**< Dados PPG prontos */
#define EVT_TEMP_READY (1 << 1)          /**< Dados temperatura prontos */
#define EVT_UI_TOUCH_START (1 << 2)      /**< Toque inicial */
#define EVT_UI_TOUCH_NEXT (1 << 3)       /**< Próximo passo */
#define EVT_UI_TOUCH_FINISH (1 << 4)     /**< Finalizar atendimento */
#define EVT_AUDIO_FINISHED (1 << 5)      /**< Áudio terminado */
#define EVT_LGPD_ACCEPTED (1 << 6)       /**< LGPD aceita */
#define EVT_LGPD_REJECTED (1 << 7)       /**< LGPD rejeitada */

/* Filas de comando para comunicação entre tasks */
QueueHandle_t xUICommandQueue;           /**< Fila de comandos para interface */
QueueHandle_t xAudioCommandQueue;        /**< Fila de comandos para áudio */

/* Sistema de áudio e arquivos WAV */
static FATFS fs;                         /**< Sistema de arquivos FAT do SD */
static FIL wav_file;                     /**< Handle de arquivo WAV */
static bool wav_file_open = false;      /**< Flag de arquivo aberto */

static int16_t wav_buffer[WAV_BUFFER_SIZE];    /**< Buffer de amostras WAV */
static uint32_t wav_buffer_position = 0;      /**< Posição atual no buffer */
static uint32_t wav_buffer_samples = 0;       /**< Número de amostras no buffer */
static uint32_t wav_data_size = 0;            /**< Tamanho dos dados WAV */
static uint32_t wav_file_position = 0;        /**< Posição atual no arquivo */

/* Parâmetros de sensores de temperatura e distância */
/* Parâmetros de sensores de temperatura e distância */
static const int calibration_offset_mm = SENSOR_CALIBRATION_OFFSET_MM;  /**< Offset calibração distância */
static const int num_samples = SENSOR_NUM_SAMPLES;                      /**< Número de amostras para média */
static const int delay_between_samples_ms = SENSOR_DELAY_BETWEEN_SAMPLES_MS; /**< Delay entre amostras */
#define IDEAL_MIN_DIST_MM 30             /**< Distância mínima ideal (mm) */
#define IDEAL_MAX_DIST_MM 50             /**< Distância máxima ideal (mm) */
#define TEMP_WINDOW_MS 10000             /**< Janela temporal para medição (ms) */
#define TEMP_OFFSET_C 4.3f               /**< Offset calibração temperatura baseado em medições reais */

/**
 * @brief Calibra leitura de temperatura com offset empírico
 * @param t_obj_c Temperatura do objeto em Celsius
 * @param t_amb_c Temperatura ambiente em Celsius (não utilizada atualmente)
 * @return Temperatura calibrada em Celsius
 * 
 * Aplica calibração linear simples adicionando offset à temperatura
 * do objeto. Assume que a leitura do MLX90614 é a medição primária.
 */
float calibrate_temp(float t_obj_c, float t_amb_c)
{
    /* Calibração linear simples: adiciona offset à temperatura do objeto */
    /* Assume que a leitura do objeto MLX90614 é a medição primária */
    return t_obj_c + TEMP_OFFSET_C;
}

bp_estimate_t last_bp = {0};             /**< Última estimativa de pressão arterial */
uint32_t last_bp_time = 0;               /**< Timestamp da última medição BP */

/* Dispositivos de sensores */
max3010x_t dev = {0};                    /**< Dispositivo MAX30102 para PPG */
bool dev_initialized = false;            /**< Flag de inicialização MAX30102 */
bool temp_initialized = false;           /**< Flag de inicialização sensores temperatura */

/**
 * @brief Inicializa I2C global uma única vez
 * 
 * Configura a porta I2C, pinos GPIO e pull-ups. Usa flag estática
 * para garantir inicialização única mesmo com múltiplas chamadas.
 */
void i2c_global_init_once(void)
{
    static bool inited = false;
    if (inited)
        return;
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    sleep_ms(200);
    inited = true;
}

/**
 * @brief Função principal do sistema
 * @return Código de saída (nunca retorna em operação normal)
 * 
 * Inicializa todos os periféricos, recursos FreeRTOS e cria as tasks
 * do sistema. Sequência de inicialização crítica:
 * 1. I2C para sensores
 * 2. SD card para logs e áudio
 * 3. UART para display Nextion
 * 4. I2S para reprodução de áudio
 * 5. WiFi para conectividade
 * 6. Recursos FreeRTOS
 * 7. Tasks do sistema
 * 8. Scheduler FreeRTOS
 */
int main(void)
{
    /* Inicializa stdio e aguarda estabilização */
    stdio_init_all();
    sleep_ms(3000);
    srand(to_ms_since_boot(get_absolute_time()));

    printf("=== SISTEMA DE TRIAGEM INTEGRADO (Pico Controla Tudo) ===\n");

    /* Inicialização I2C global */
    printf("Inicializando I2C...\n");
    i2c_global_init_once();

    /* SD card deve ser montado primeiro para logs e áudio */
    printf("Montando SD card...\n");
    if (f_mount(&fs, "", 1) != FR_OK)
    {
        printf("ERRO CRÍTICO: Falha ao montar o cartão SD.\n");
        while (true)
        {
            tight_loop_contents();
        }
    }
    printf("Cartão SD montado.\n");

    /* Configuração UART para display Nextion */
    printf("Configurando UART Nextion...\n");
    nextion_init();
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    printf("UART Nextion pronta (UART%d, TX=%d, RX=%d).\n",
           uart_get_index(UART_ID), UART_TX_PIN, UART_RX_PIN);

    /* Configuração I2S com fallback para múltiplos canais DMA/PIO */
    printf("Configurando I2S...\n");
    audio_i2s_config_t config = {
        .data_pin = I2S_DATA_PIN,
        .clock_pin_base = I2S_CLOCK_PIN_BASE,
        .dma_channel =  2,
        .pio_sm = 2,
    };

    const audio_format_t *output_format = audio_i2s_setup(&audio_format, &config);
    if (!output_format)
    {
       
        printf("Tentativa 1 (DMA 2, PIO 2) falhou. Tentando DMA 3, PIO 3...\n");
        config.dma_channel = 3;
        config.pio_sm = 3;
        output_format = audio_i2s_setup(&audio_format, &config);
    }
    if (!output_format)
    {
        printf("ERRO CRÍTICO: falha ao configurar I2S.\n");
        while (true)
        {
            tight_loop_contents();
        }
    }
    printf("I2S OK (DMA %d, PIO %d).\n", config.dma_channel, config.pio_sm);

    /* Cria pool de buffers I2S para áudio estéreo (stride 4 bytes) */
    /* Cria pool de buffers I2S para áudio estéreo (stride 4 bytes) */
    audio_buffer_format_t producer_format = {
        .format = output_format,
        .sample_stride = 4};
    producer_pool = audio_new_producer_pool(&producer_format, 3, SAMPLES_PER_BUFFER);
    audio_i2s_connect(producer_pool);
    audio_i2s_set_enabled(true);
    printf("Sistema de Áudio pronto.\n");

    /* Inicialização WiFi para conectividade */
    printf("Iniciando WiFi...\n");
    wifi_start();

    /* Criação de recursos FreeRTOS */
    printf("Criando recursos RTOS...\n");
    xEvents = xEventGroupCreate();
    xI2CMutex = xSemaphoreCreateMutex();
    xDataQueue = xQueueCreate(10, sizeof(paciente_t));
    xUICommandQueue = xQueueCreate(10, sizeof(task_command_t));
    xAudioCommandQueue = xQueueCreate(5, sizeof(task_command_t));

    if (!xEvents || !xI2CMutex || !xDataQueue || !xUICommandQueue || !xAudioCommandQueue)
    {
        printf("ERRO: Falha ao criar recursos RTOS!\n");
        while (true)
            tight_loop_contents();
    }
    printf("Recursos RTOS criados com sucesso\n");

    /* Criação de tasks com stacks otimizados para economia de memória */
    printf("Criando tasks...\n");

    BaseType_t task_results[7];
    task_results[0] = xTaskCreate(TaskUI, "UI", 1536, NULL, 3, NULL);        /* UI: 1536 bytes stack */
    task_results[1] = xTaskCreate(TaskAudio, "Audio", 1536, NULL, 4, NULL);  /* Audio: 1536 bytes stack */
    task_results[2] = xTaskCreate(TaskFSM, "FSM", 2048, NULL, 5, NULL);      /* FSM: 2048 bytes stack */
    task_results[3] = xTaskCreate(TaskPPG, "PPG", 2048, NULL, 2, NULL);      /* PPG: 2048 bytes stack */
    task_results[4] = xTaskCreate(TaskTempDist, "TempDist", 1536, NULL, 2, NULL); /* TempDist: 1536 bytes stack */
    task_results[5] = xTaskCreate(TaskServer, "Server", 1024, NULL, 1, NULL); /* Server: 1024 bytes stack */
    task_results[6] = xTaskCreate(TaskWiFiCheck, "WiFiChk", 1024, NULL, 1, NULL); /* WiFiCheck: 1024 bytes stack */

    /* Verifica se todas as tasks foram criadas com sucesso */
    bool all_tasks_ok = true;
    for (int i = 0; i < 7; i++)
    {
        if (task_results[i] != pdPASS)
        {
            printf("ERRO: Task %d falhou!\n", i);
            all_tasks_ok = false;
        }
    }

    if (!all_tasks_ok)
    {
        printf("ERRO CRÍTICO: Falha ao criar tasks!\n");
        while (true)
            tight_loop_contents();
    }

    printf("Tasks criadas com sucesso!\n");
    printf("🚀 Iniciando scheduler...\n");

    /* Inicia o scheduler FreeRTOS (nunca retorna em operação normal) */
    vTaskStartScheduler();

    /* Se chegar aqui, indica falha crítica de memória heap/stack */
    printf("ERRO CRÍTICO: Scheduler retornou!\n");
    while (true)
    {
        tight_loop_contents();
    }
}