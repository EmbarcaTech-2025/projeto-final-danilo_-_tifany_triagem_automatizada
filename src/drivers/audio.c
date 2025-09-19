/**
 * @file audio.c
 * @brief Implementação do driver de áudio I2S para reprodução WAV
 * 
 * Este arquivo implementa as funções para controle de áudio usando interface I2S,
 * incluindo inicialização, reprodução de arquivos WAV em streaming e controle
 * de buffers de áudio.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#include "audio.h"
#include "pico/audio_i2s.h"
#include "hardware/dma.h"
#include "hardware/pio.h"
#include "ff.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include <stdio.h>
#include "utils/events.h"

/* Variáveis globais do sistema de áudio */
audio_buffer_pool_t *producer_pool = NULL;
volatile bool wav_playing = false;

/* Estrutura de formato de áudio configurada para estéreo PCM 16-bit */
audio_format_t audio_format = {
    .format = AUDIO_BUFFER_FORMAT_PCM_S16,
    .sample_freq = SAMPLE_RATE,
    .channel_count = 2,
};

/* Estado interno do reprodutor WAV */
static FIL wav_file;
static bool wav_file_open = false;
static int16_t wav_buffer[WAV_BUFFER_SIZE];
static uint32_t wav_buffer_position = 0;
static uint32_t wav_buffer_samples = 0;
static uint32_t wav_data_size = 0;
static uint32_t wav_file_position = 0;

/* Grupo de eventos externa (declarada no main.c) */
extern EventGroupHandle_t xEvents;

bool audio_init(void)
{
    printf("Configurando I2S...\n");
    
    /* Configuração inicial do I2S com canal DMA 2 e PIO SM 2 */
    audio_i2s_config_t config = {
        .data_pin = I2S_DATA_PIN,
        .clock_pin_base = I2S_CLOCK_PIN_BASE,
        .dma_channel = 2,
        .pio_sm = 2,
    };

    /* Tenta configurar I2S com os parâmetros iniciais */
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
        return false;
    }
    printf("I2S OK (DMA %d, PIO %d).\n", config.dma_channel, config.pio_sm);

    /* Cria pool de buffers I2S para áudio estéreo (4 bytes por amostra) */
    audio_buffer_format_t producer_format = {
        .format = output_format,
        .sample_stride = 4
    };
    producer_pool = audio_new_producer_pool(&producer_format, 3, SAMPLES_PER_BUFFER);
    audio_i2s_connect(producer_pool);
    audio_i2s_set_enabled(true);
    printf("Sistema de Áudio pronto.\n");
    
    return true;
}

bool open_wav_for_streaming(const char *filename)
{
    /* Tenta abrir o arquivo WAV no sistema de arquivos */
    FRESULT fr = f_open(&wav_file, filename, FA_READ);
    if (fr != FR_OK)
    {
        printf("ERRO: Arquivo %s não encontrado\n", filename);
        return false;
    }
    wav_file_open = true;
    
    /* Pula o cabeçalho WAV padrão (44 bytes) */
    f_lseek(&wav_file, 44);

    /* Calcula tamanho dos dados de áudio */
    uint32_t data_start = f_tell(&wav_file);
    wav_data_size = f_size(&wav_file) - data_start;
    wav_file_position = 0;

    /* Carrega primeiro buffer de dados */
    UINT bytes_read;
    f_read(&wav_file, wav_buffer, sizeof(wav_buffer), &bytes_read);
    wav_buffer_samples = bytes_read / sizeof(int16_t);
    wav_buffer_position = 0;
    wav_file_position += bytes_read;

    printf("Áudio aberto: %s (%lu bytes)\n", filename, wav_data_size);
    return true;
}

bool refill_wav_buffer(void)
{
    if (!wav_file_open)
        return false;

    /* Verifica se ainda há dados para ler */
    uint32_t remaining = wav_data_size - wav_file_position;
    if (remaining == 0)
    {
        wav_playing = false;
        f_close(&wav_file);
        wav_file_open = false;

        /* Notifica que o áudio terminou via evento */
        if (xEvents) {
            xEventGroupSetBits(xEvents, EVT_AUDIO_FINISHED);
        }
        printf("Áudio finalizado\n");
        return false;
    }

    /* Lê próximo bloco de dados do arquivo */
    UINT to_read = (remaining < sizeof(wav_buffer)) ? remaining : sizeof(wav_buffer);
    UINT bytes_read;
    f_read(&wav_file, wav_buffer, to_read, &bytes_read);

    wav_buffer_samples = bytes_read / sizeof(int16_t);
    wav_buffer_position = 0;
    wav_file_position += bytes_read;
    return true;
}

void fill_audio_buffer(audio_buffer_t *buffer)
{
    int16_t *samples = (int16_t *)buffer->buffer->bytes;
    buffer->sample_count = buffer->max_sample_count;

    /* Preenche buffer com amostras de áudio */
    for (uint32_t i = 0; i < buffer->max_sample_count; i++)
    {
        int16_t sample = 0;

        if (wav_playing)
        {
            /* Verifica se há amostras disponíveis no buffer */
            if (wav_buffer_position < wav_buffer_samples)
            {
                sample = wav_buffer[wav_buffer_position++];
            }
            else
            {
                /* Tenta recarregar buffer se necessário */
                if (!refill_wav_buffer())
                {
                    wav_playing = false;
                }
            }
        }

        /* Aplica volume e duplica para canal estéreo */
        int16_t final_sample = (int16_t)(((int32_t)sample * VOLUME) / 32767);
        samples[i * 2 + 0] = final_sample; /* Canal esquerdo */
        samples[i * 2 + 1] = final_sample; /* Canal direito */
    }
}

void play_audio_file(const char *filename)
{
    /* Para reprodução atual se houver */
    if (wav_playing)
    {
        wav_playing = false;
    }
    if (wav_file_open)
    {
        f_close(&wav_file);
        wav_file_open = false;
    }

    printf("Reproduzindo: %s\n", filename);
    if (open_wav_for_streaming(filename))
    {
        wav_playing = true;
    }
}

void audio_service_once(void)
{
    if (!producer_pool)
        return; /* Sistema de áudio desabilitado */

    /* Obtém buffer disponível e o preenche com dados de áudio */
    audio_buffer_t *buffer = take_audio_buffer(producer_pool, false);
    if (buffer)
    {
        fill_audio_buffer(buffer);
        give_audio_buffer(producer_pool, buffer);
    }
}

void audio_stop(void)
{
    wav_playing = false;
    if (wav_file_open)
    {
        f_close(&wav_file);
        wav_file_open = false;
    }
}
