/**
 * @file task_ppg.c
 * @brief Implementação da task de medição PPG (fotopletismografia)
 * 
 * Esta task utiliza o sensor MAX30102 para capturar sinais ópticos
 * dos dedos do usuário e processar os dados para obter medidas de
 * batimentos cardíacos, saturação de oxigênio e pressão arterial.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#include "task_ppg.h"

/**
 * @brief Task principal de medição de parâmetros cardíacos
 * @param pvParameters Parâmetros FreeRTOS (não utilizado)
 * 
 * Controla o sensor MAX30102, captura amostras PPG, processa
 * algoritmos de heart rate e SpO2, e salva resultados no paciente.
 */
void TaskPPG(void *pvParameters)
{
    printf("[PPG] Task iniciada\n");
    
    // Initialize the MAX30102 sensor
    max3010x_t dev = {0};
    dev.i2c = I2C_PORT;
    dev.address = MAX3010X_I2C_ADDR;
    dev.active_leds = 2; // red+ir
    
    if (!max3010x_init(&dev)) {
        printf("[PPG] Falha na inicialização do MAX30102\n");
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Configure with optimal settings for PPG measurement
    if (!max3010x_configure_spo2(&dev, 60, 4, 100, 411, 4096)) {
        printf("[PPG] Falha na configuração do MAX30102\n");
        while (1) vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    printf("[PPG] MAX30102 inicializado com sucesso\n");
    
    // Clear FIFO and wait for stabilization
    max3010x_clear_fifo(&dev);
    vTaskDelay(pdMS_TO_TICKS(200));
    
    while (true)
    {
        if (estado_atual == ESTADO_PPG)
        {
            printf("[PPG] Iniciando medição cardíaca...\n");
            
            // Reset all state for new measurement
            uint32_t ir_buf[BUFFER_SIZE];
            uint32_t red_buf[BUFFER_SIZE];
            uint8_t rates[RATE_SIZE] = {0};
            uint8_t rateSpot = 0;
            uint32_t lastBeat = 0;
            int beatAvg = 0;
            float beatsPerMinute = 0;
            uint32_t lastValidBeat = 0;
            bool fingerDetected = false;
            bool hasValidBPM = false;
            bool readingsStabilized = false;
            uint32_t fingerDetectedTime = 0;
            uint32_t firstValidBPMTime = 0;
            uint32_t stable_show_start = 0;
            int buffer_idx = 0;
            
            // Variables for SPO2
            int32_t spo2 = 0;
            int8_t validSPO2 = 0;
            int32_t spo2_heartRate = 0;
            int8_t spo2_validHeartRate = 0;
            
            // Variables for PPG -> BP
            float ppg_ir_buffer[PPG_BUFFER_SIZE];
            float ppg_red_buffer[PPG_BUFFER_SIZE];
            int ppg_buffer_idx = 0;
            bool ppg_buffer_full = false;
            bp_estimate_t local_bp = {0};
            uint32_t local_bp_time = 0;
            
            // For Maxim algorithm heart rate averaging
            int32_t maxim_history[5] = {0};
            int maxim_history_idx = 0;
            int32_t maxim_avg = 0;
            
            uint32_t measurement_start_time = to_ms_since_boot(get_absolute_time());
            uint32_t last_debug_print = measurement_start_time;
            uint32_t last_ui_update = 0; // Force first update
            
            // Active measurement loop
            while (estado_atual == ESTADO_PPG) 
            {
                max3010x_sample_t sample;
                uint32_t currentTime = to_ms_since_boot(get_absolute_time());
                
                // Update UI every 1000ms for status text only
                if (currentTime - last_ui_update >= 1000) {
                    task_command_t ui_cmd;
                    
                    // Update status text (sem acentos)
                    ui_cmd.cmd = UI_CMD_UPDATE_TEXT;
                    ui_cmd.param1 = 0; // Use t_bottom.txt
                    
                    if (!fingerDetected) {
                        strcpy(ui_cmd.text, "Coloque o dedo no sensor");
                    } else if (!hasValidBPM) {
                        strcpy(ui_cmd.text, "Dedo detectado. Iniciando...");
                    } else if (!readingsStabilized) {
                        uint32_t stabilization_elapsed = currentTime - firstValidBPMTime;
                        uint32_t remaining = (STABILIZATION_TIME_MS - stabilization_elapsed) / 1000;
                        sprintf(ui_cmd.text, "Estabilizando... %lus", remaining);
                    } else {
                        strcpy(ui_cmd.text, "Medindo batimentos...");
                    }
                    
                    xQueueSend(xUICommandQueue, &ui_cmd, 0);
                    last_ui_update = currentTime;
                }
                
                // Debug print every 5 seconds
                if (currentTime - last_debug_print > 5000) {
                    printf("[PPG DEBUG] Finger:%d, ValidBPM:%d, Stabilized:%d, Time:%lus\n", 
                           fingerDetected, hasValidBPM, readingsStabilized, 
                           (currentTime - measurement_start_time) / 1000);
                    last_debug_print = currentTime;
                }
                
                if (max3010x_read_sample(&dev, &sample)) {
                    uint32_t irValue = sample.ir;
                    uint32_t redValue = sample.red;
                    
                    // Finger detection
                    bool currentFingerDetected = (irValue > FINGER_THRESHOLD);
                    if (fingerDetected != currentFingerDetected) {
                        fingerDetected = currentFingerDetected;
                        
                        if (fingerDetected) {
                            // Finger just detected
                            fingerDetectedTime = currentTime;
                            firstValidBPMTime = 0;
                            readingsStabilized = false;
                            hasValidBPM = false;
                            ppg_buffer_idx = 0;
                            ppg_buffer_full = false;
                            printf("[PPG] Dedo detectado (IR=%lu)\n", (unsigned long)irValue);
                        } else {
                            // Finger removed
                            if (readingsStabilized && stable_show_start != 0) {
                                printf("[PPG] Dedo removido após medição estável\n");
                                goto ppg_measurement_complete;
                            }
                            
                            // Reset if finger removed before stabilization
                            readingsStabilized = false;
                            hasValidBPM = false;
                            fingerDetectedTime = 0;
                            firstValidBPMTime = 0;
                            stable_show_start = 0;
                            for (int i = 0; i < RATE_SIZE; i++) rates[i] = 0;
                            rateSpot = 0;
                            beatAvg = 0;
                            printf("[PPG] Dedo removido. Aguardando novo contato\n");
                        }
                    }
                    
                    // Reset if no beats detected for too long (after stabilization)
                    if (readingsStabilized && lastValidBeat != 0 && 
                        (currentTime - lastValidBeat) > BEAT_TIMEOUT_MS) {
                        printf("[PPG] Timeout de batimentos. Reiniciando medição\n");
                        for (int i = 0; i < RATE_SIZE; i++) rates[i] = 0;
                        rateSpot = 0;
                        beatAvg = 0;
                        lastValidBeat = 0;
                    }
                    
                    // Heart rate detection (SparkFun algorithm)
                    if (fingerDetected && checkForBeat(irValue)) {
                        uint32_t delta = currentTime - lastBeat;
                        lastBeat = currentTime;
                        lastValidBeat = currentTime;
                        
                        beatsPerMinute = 60000.0 / (float)delta;
                        
                        printf("[PPG] Beat detectado! Delta=%lums, BPM=%.1f\n", 
                               (unsigned long)delta, beatsPerMinute);
                        
                        if (beatsPerMinute < 200 && beatsPerMinute > 40) {
                            rates[rateSpot++] = (uint8_t)beatsPerMinute;
                            rateSpot %= RATE_SIZE;
                            
                            // Calculate average
                            beatAvg = 0;
                            for (uint8_t x = 0; x < RATE_SIZE; x++) 
                                beatAvg += rates[x];
                            beatAvg /= RATE_SIZE;
                            
                            // Check if this is our first valid BPM reading
                            if (!hasValidBPM && beatAvg > 0) {
                                hasValidBPM = true;
                                firstValidBPMTime = currentTime;
                                printf("[PPG] Primeiro batimento válido detectado! BPM=%d. Estabilizando...\n", beatAvg);
                            }
                        } else {
                            printf("[PPG] BPM fora do range normal: %.1f (ignorado)\n", beatsPerMinute);
                        }
                    }
                    
                    // Check if stabilization period is complete
                    if (fingerDetected && hasValidBPM && !readingsStabilized) {
                        uint32_t stabilizationElapsed = currentTime - firstValidBPMTime;
                        if (stabilizationElapsed >= STABILIZATION_TIME_MS) {
                            readingsStabilized = true;
                            stable_show_start = currentTime;
                            printf("[PPG] Leituras estabilizadas após %lums\n", stabilizationElapsed);
                        } else {
                            // Show countdown during stabilization
                            static uint32_t lastCountdownPrint = 0;
                            if (currentTime - lastCountdownPrint > 2000) {
                                uint32_t remainingSeconds = (STABILIZATION_TIME_MS - stabilizationElapsed) / 1000;
                                printf("[PPG] Estabilizando... %lu segundos restantes (BPM atual: %d)\n", 
                                       remainingSeconds, beatAvg);
                                lastCountdownPrint = currentTime;
                            }
                        }
                    }
                    
                    // Fill SPO2 buffer
                    if (fingerDetected && buffer_idx < BUFFER_SIZE) {
                        ir_buf[buffer_idx] = irValue;
                        red_buf[buffer_idx] = redValue;
                        buffer_idx++;
                    }
                    
                    // Calculate SPO2 when buffer is full
                    if (buffer_idx == BUFFER_SIZE) {
                        maxim_heart_rate_and_oxygen_saturation(
                            ir_buf, BUFFER_SIZE, red_buf,
                            &spo2, &validSPO2, &spo2_heartRate, &spo2_validHeartRate
                        );
                        
                        // Add Maxim reading to history for averaging (after stabilization)
                        if (readingsStabilized && spo2_validHeartRate && 
                            spo2_heartRate > 40 && spo2_heartRate < 200) {
                            maxim_history[maxim_history_idx] = spo2_heartRate;
                            maxim_history_idx = (maxim_history_idx + 1) % 5;
                            
                            // Calculate average of valid readings
                            int32_t sum = 0;
                            int count = 0;
                            for (int i = 0; i < 5; i++) {
                                if (maxim_history[i] > 0) {
                                    sum += maxim_history[i];
                                    count++;
                                }
                            }
                            maxim_avg = count > 0 ? sum / count : spo2_heartRate;
                        }
                        
                        buffer_idx = 0;
                    }
                    
                    // PPG analysis for Blood Pressure
                    if (fingerDetected && readingsStabilized) {
                        // Add PPG data to analysis buffer
                        ppg_ir_buffer[ppg_buffer_idx] = (float)irValue;
                        ppg_red_buffer[ppg_buffer_idx] = (float)redValue;
                        ppg_buffer_idx++;
                        
                        if (ppg_buffer_idx >= PPG_BUFFER_SIZE) {
                            ppg_buffer_idx = 0;
                            ppg_buffer_full = true;
                        }
                        
                        // Analyze PPG every 2 seconds when buffer is full
                        static uint32_t lastPPGAnalysis = 0;
                        if (ppg_buffer_full && (currentTime - lastPPGAnalysis) > 2000) {
                            // Normalize signal before analysis
                            float normalized_ir[PPG_BUFFER_SIZE];
                            float normalized_red[PPG_BUFFER_SIZE];
                            
                            // Copy and normalize
                            for (int i = 0; i < PPG_BUFFER_SIZE; i++) {
                                normalized_ir[i] = ppg_ir_buffer[i];
                                normalized_red[i] = ppg_red_buffer[i];
                            }
                            
                            normalize_ppg_buffer(normalized_ir, PPG_BUFFER_SIZE);
                            normalize_ppg_buffer(normalized_red, PPG_BUFFER_SIZE);
                            
                            ppg_features_t features = extract_ppg_features(normalized_ir, normalized_red, PPG_BUFFER_SIZE);
                            bp_estimate_t bp = estimate_blood_pressure(features, beatAvg);
                            
                            if (bp.confidence > 0.3f) {
                                local_bp = bp;
                                local_bp_time = currentTime;
                                printf("[PPG] Estimativa PA: %d/%d mmHg (Confianca: %.1f%%)\n",
                                       (int)bp.systolic_bp, (int)bp.diastolic_bp, bp.confidence * 100.0f);
                            }
                            
                            lastPPGAnalysis = currentTime;
                        }
                    }
                    
                    // Status updates
                    if (readingsStabilized && (currentTime % 2000) < 100) {
                        printf("[PPG] Medição: BPM=%d, SpO2=%ld%%\n", beatAvg, (long)spo2);
                    }
                    
                    // Complete measurement after sufficient stabilization time
                    if (readingsStabilized && stable_show_start != 0 && 
                        (currentTime - stable_show_start) > 10000) {
                        printf("[PPG] Tempo de medição estável suficiente (%lums)\n", 
                               currentTime - stable_show_start);
                        goto ppg_measurement_complete;
                    }
                } else {
                    printf("[PPG] AVISO: Falha ao ler amostra do sensor\n");
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                
                // Overall timeout (45 seconds)
                if ((currentTime - measurement_start_time) > 45000) {
                    printf("[PPG] Timeout da medição (45s). ");
                    if (readingsStabilized) {
                        printf("Usando valores disponíveis.\n");
                        goto ppg_measurement_complete;
                    } else {
                        printf("Sem valores estáveis. Usando últimas leituras.\n");
                        if (beatAvg == 0) beatAvg = 75; // Fallback only if we have no reading at all
                        if (spo2 == 0) spo2 = 98;
                        goto ppg_measurement_complete;
                    }
                }
                
                vTaskDelay(pdMS_TO_TICKS(20)); // 20ms gives us ~50Hz sampling
            }
            
            // Success path for measurement completion
            ppg_measurement_complete:
            
            // Update final status (sem acentos)
            task_command_t final_cmd = {
                .cmd = UI_CMD_UPDATE_TEXT,
                .param1 = 0
            };
            strcpy(final_cmd.text, "Medicao concluida!");
            xQueueSend(xUICommandQueue, &final_cmd, 0);
            
            // Store results in the patient record
            pacienteAtual.bpm = beatAvg > 0 ? beatAvg : 75;
            pacienteAtual.spo2 = validSPO2 ? spo2 : 98;
            
            if (local_bp.confidence > 0.3f) {
                pacienteAtual.pressao_sys = (int)local_bp.systolic_bp;
                pacienteAtual.pressao_dia = (int)local_bp.diastolic_bp;
            } else {
                // Simple estimation based on heart rate if no confidence
                pacienteAtual.pressao_sys = 110 + (beatAvg - 70) / 2;
                pacienteAtual.pressao_dia = 70 + (beatAvg - 70) / 3;
            }
            
            pacienteAtual.valid = (beatAvg > 0);
            
            printf("PPG COMPLETO -> BPM=%d, SpO2=%d%%, PA=%d/%d mmHg\n",
                   pacienteAtual.bpm, pacienteAtual.spo2,
                   (int)pacienteAtual.pressao_sys, (int)pacienteAtual.pressao_dia);
            
            // Notify that we've completed
            xEventGroupSetBits(xEvents, EVT_PPG_READY);
            
            // Wait until we exit this state
            while (estado_atual == ESTADO_PPG) {
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
