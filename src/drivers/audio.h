/**
 * @file audio.h
 * @brief Driver para controle de áudio I2S com reprodução de arquivos WAV
 * 
 * Este módulo implementa um driver de áudio para reprodução de arquivos WAV
 * utilizando interface I2S no Raspberry Pi Pico. Suporta reprodução em streaming
 * para economia de memória RAM.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#ifndef AUDIO_H
#define AUDIO_H

#include "pico/stdlib.h"
#include "pico/audio_i2s.h"
#include "ff.h"
#include <stdbool.h>
#include <stdint.h>

/* Configuração do sistema I2S */
#define SAMPLE_RATE 24000           /**< Taxa de amostragem em Hz */
#define SAMPLES_PER_BUFFER 256      /**< Amostras por buffer */
#define VOLUME 8000                 /**< Volume padrão */
#define I2S_DATA_PIN 20            /**< Pino GPIO para dados I2S */
#define I2S_CLOCK_PIN_BASE 8       /**< Pino base para clock I2S */

/* Configuração do buffer WAV */
#define WAV_BUFFER_SIZE 1024       /**< Tamanho do buffer WAV em amostras */

/* Variáveis globais do sistema de áudio */
extern audio_buffer_pool_t *producer_pool;  /**< Pool de buffers I2S */
extern volatile bool wav_playing;           /**< Flag de reprodução ativa */
extern audio_format_t audio_format;         /**< Formato de áudio configurado */

/* Declarações das funções públicas */

/**
 * @brief Inicializa o sistema de áudio I2S
 * @return true se inicialização bem-sucedida, false caso contrário
 */
bool audio_init(void);

/**
 * @brief Abre arquivo WAV para reprodução em streaming
 * @param filename Nome do arquivo WAV a ser reproduzido
 * @return true se arquivo aberto com sucesso, false caso contrário
 */
bool open_wav_for_streaming(const char *filename);

/**
 * @brief Recarrega o buffer WAV do arquivo
 * @return true se dados foram carregados, false no fim do arquivo
 */
bool refill_wav_buffer(void);

/**
 * @brief Preenche buffer de áudio com dados WAV
 * @param buffer Ponteiro para buffer de áudio a ser preenchido
 */
void fill_audio_buffer(audio_buffer_t *buffer);

/**
 * @brief Inicia reprodução de arquivo de áudio
 * @param filename Nome do arquivo a ser reproduzido
 */
void play_audio_file(const char *filename);

/**
 * @brief Processa buffers de áudio (deve ser chamado periodicamente)
 */
void audio_service_once(void);

/**
 * @brief Para a reprodução de áudio atual
 */
void audio_stop(void);

#endif // AUDIO_H
