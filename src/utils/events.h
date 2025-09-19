/**
 * @file events.h
 * @brief Definições de eventos do sistema para comunicação entre tasks
 * 
 * Este arquivo centraliza todas as definições de eventos utilizados pelo
 * sistema FreeRTOS para comunicação e sincronização entre as diferentes
 * tasks do sistema de triagem.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#ifndef EVENTS_H
#define EVENTS_H

#include "FreeRTOS.h"
#include "event_groups.h"

extern EventGroupHandle_t xEvents;

/* Bits de eventos para controle de fluxo do sistema */
#define EVT_PPG_READY       (1 << 0)   /**< Dados de PPG/oximetria prontos */
#define EVT_TEMP_READY      (1 << 1)   /**< Dados de temperatura prontos */
#define EVT_UI_TOUCH_START  (1 << 2)   /**< Toque inicial na tela */
#define EVT_UI_TOUCH_NEXT   (1 << 3)   /**< Toque para próxima etapa */
#define EVT_UI_TOUCH_FINISH (1 << 4)   /**< Toque para finalizar */
#define EVT_AUDIO_FINISHED  (1 << 5)   /**< Reprodução de áudio finalizada */
#define EVT_LGPD_ACCEPTED   (1 << 6)   /**< LGPD aceita pelo usuário */
#define EVT_LGPD_REJECTED   (1 << 7)   /**< LGPD rejeitada pelo usuário */

#endif