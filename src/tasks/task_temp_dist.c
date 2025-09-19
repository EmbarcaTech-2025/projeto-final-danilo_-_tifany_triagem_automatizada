/**
 * @file task_temp_dist.c
 * @brief Implementação da task de medição de temperatura e distância
 * 
 * Esta task controla sensores I2C (MLX90614 para temperatura infravermelha
 * e VL53L0X para distância laser) realizando medições calibradas de
 * temperatura corporal com validação de posicionamento correto.
 * 
 * @author Danilo Oliveira e Tífany Severo
 * @date 2025
 */

#include "task_temp_dist.h"

/**
 * @brief Task principal de medição de temperatura e distância
 * @param pvParameters Parâmetros FreeRTOS (não utilizado)
 * 
 * Inicializa sensores I2C, realiza medições de temperatura com
 * controle de distância, aplica calibrações e salva resultados.
 */
void TaskTempDist(void *pvParameters)
{
    printf("[TEMP] Task iniciada\n");
    
    while (true)
    {
        if (estado_atual == ESTADO_TEMP)
        {
            printf("[TEMP] Iniciando medição de temperatura...\n");
            
            bool temp_sensor_working = false;
            bool dist_sensor_working = false;
            bool distancia_audio_played = false; // Flag para tocar áudio de "distância OK" apenas uma vez
            bool teve_medicao_correta = false;    // NOVA: Flag para indicar se já teve pelo menos uma medição correta
            uint32_t last_distance_audio_time = 0; // Controle de intervalo dos áudios de distância
            const uint32_t DISTANCE_AUDIO_INTERVAL_MS = 5000; // 5 segundos entre áudios
            
            // Try to initialize temperature sensors
            if (!temp_initialized) {
                // Initialize I2C
                i2c_init(I2C_PORT, 100 * 1000);
                gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
                gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
                gpio_pull_up(I2C_SDA_PIN);
                gpio_pull_up(I2C_SCL_PIN);
                vTaskDelay(pdMS_TO_TICKS(200));
                
                // Try MLX90614
                if (mlx90614_init(I2C_PORT, MLX90614_DEFAULT_ADDR) == 1) {
                    printf("[TEMP] ✅ MLX90614 inicializado\n");
                    temp_sensor_working = true;
                    temp_initialized = true;
                } else {
                    printf("[TEMP] ⚠️ MLX90614 falhou - usando simulação\n");
                }
                
                // Try VL53L0X for distance measurement
                if (vl53l0x_init(0, 0x29, 0) == 1) {
                    printf("[TEMP] ✅ VL53L0X inicializado\n");
                    dist_sensor_working = true;
                } else {
                    printf("[TEMP] ⚠️ VL53L0X falhou - usando distância fixa\n");
                }
            } else {
                temp_sensor_working = true;
                // Test VL53L0X again
                int test_distance = vl53l0x_read_distance();
                if (test_distance > 0) {
                    dist_sensor_working = true;
                }
            }
            
            if (temp_sensor_working) {
                // Try real temperature measurement with distance guidance
                printf("[TEMP] Tentando medição real por 60 segundos...\n");
                
                float temp_readings[30];
                int valid_readings = 0;
                uint32_t temp_start = to_ms_since_boot(get_absolute_time());
                uint32_t last_hint = 0;
                uint32_t last_ui_update = 0; // Force first update
                
                // Try to get real readings for 60 seconds
                while (estado_atual == ESTADO_TEMP && 
                       (to_ms_since_boot(get_absolute_time()) - temp_start) < 60000 &&
                       valid_readings < 30) {
                    
                    uint32_t current_time = to_ms_since_boot(get_absolute_time());
                    
                    // Update UI every 1000ms for status text only
                    if (current_time - last_ui_update >= 1000) {
                        last_ui_update = current_time;
                    }
                    
                    // Check distance if VL53L0X is working
                    bool distance_ok = true;
                    if (dist_sensor_working) {
                        int raw_distance = vl53l0x_read_distance();
                        if (raw_distance > 0) {
                            int calibrated_distance = raw_distance - SENSOR_CALIBRATION_OFFSET_MM;
                            if (calibrated_distance < 0) calibrated_distance = 0;
                            
                            // Update UI with distance feedback (sem acentos)
                            task_command_t ui_cmd;
                            ui_cmd.cmd = UI_CMD_UPDATE_TEXT;
                            ui_cmd.param1 = 0; // Use t_bottom.txt
                            
                            // Check if distance is in ideal range
                            if (calibrated_distance < IDEAL_MIN_DIST_MM) {
                                distance_ok = false;
                                strcpy(ui_cmd.text, "Muito perto! Afaste um pouco");
                                distancia_audio_played = false; // Reset flag quando sair da distância ideal
                                
                                // Toca áudio "afastar" com intervalo APENAS se já teve medição correta
                                if (teve_medicao_correta && (current_time - last_distance_audio_time) >= DISTANCE_AUDIO_INTERVAL_MS) {
                                    task_command_t audio_cmd = {
                                        .cmd = AUDIO_CMD_PLAY,
                                        .param1 = 9
                                    };
                                    strcpy(audio_cmd.filename, "afastar.wav");
                                    xQueueSend(xAudioCommandQueue, &audio_cmd, 0);
                                    last_distance_audio_time = current_time;
                                    printf("[TEMP] 🔊 Áudio 'afastar' reproduzido (após medição correta)\n");
                                }
                                
                            } else if (calibrated_distance > IDEAL_MAX_DIST_MM) {
                                distance_ok = false;
                                strcpy(ui_cmd.text, "Muito longe! Aproxime um pouco");
                                distancia_audio_played = false; // Reset flag quando sair da distância ideal
                                
                                // Toca áudio "aproximar" com intervalo APENAS se já teve medição correta
                                if (teve_medicao_correta && (current_time - last_distance_audio_time) >= DISTANCE_AUDIO_INTERVAL_MS) {
                                    task_command_t audio_cmd = {
                                        .cmd = AUDIO_CMD_PLAY,
                                        .param1 = 10
                                    };
                                    strcpy(audio_cmd.filename, "aproximar.wav");
                                    xQueueSend(xAudioCommandQueue, &audio_cmd, 0);
                                    last_distance_audio_time = current_time;
                                    printf("[TEMP] 🔊 Áudio 'aproximar' reproduzido (após medição correta)\n");
                                }
                                
                            } else {
                                pacienteAtual.distancia_mm = calibrated_distance;
                                strcpy(ui_cmd.text, "Distancia OK - Medindo...");
                                
                                // Toca áudio de distância correta apenas uma vez (sempre, mesmo sem medição prévia)
                                if (!distancia_audio_played) {
                                    task_command_t audio_cmd = {
                                        .cmd = AUDIO_CMD_PLAY,
                                        .param1 = 8
                                    };
                                    strcpy(audio_cmd.filename, "distancia.wav");
                                    xQueueSend(xAudioCommandQueue, &audio_cmd, 0);
                                    distancia_audio_played = true;
                                    printf("[TEMP] ✅ Áudio de distância correta reproduzido\n");
                                }
                            }
                            
                            xQueueSend(xUICommandQueue, &ui_cmd, 0);
                            
                            // Give user feedback occasionally to console
                            if ((current_time - last_hint) > 2000 && !distance_ok) {
                                printf("[TEMP] Ajuste distância: atual=%dmm, ideal=%d-%dmm\n", 
                                       calibrated_distance, IDEAL_MIN_DIST_MM, IDEAL_MAX_DIST_MM);
                                last_hint = current_time;
                            }
                        }
                    } else {
                        // No distance sensor, just show progress
                        task_command_t ui_cmd;
                        ui_cmd.cmd = UI_CMD_UPDATE_TEXT;
                        ui_cmd.param1 = 0;
                        strcpy(ui_cmd.text, "Medindo temperatura...");
                        xQueueSend(xUICommandQueue, &ui_cmd, 0);
                        distance_ok = true; // Assume OK if no distance sensor
                    }
                    
                    if (distance_ok) {
                        float t_obj_c = 0.0f, t_amb_c = 0.0f;
                        if (mlx90614_read_object_c(&t_obj_c) && mlx90614_read_ambient_c(&t_amb_c)) {
                            float calibrated = calibrate_temp(t_obj_c, t_amb_c);
                            
                            // Sanity check
                            if (calibrated > 30.0f && calibrated < 45.0f) {
                                temp_readings[valid_readings++] = calibrated;
                                teve_medicao_correta = true; // MARCA que já teve pelo menos uma medição correta
                                printf("[TEMP] Leitura %d: %.1f°C (dist=%dmm) [medição correta habilitada]\n", 
                                       valid_readings, calibrated, pacienteAtual.distancia_mm);
                            }
                        }
                    }
                    
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
                
                if (valid_readings >= 20) {
                    // Calculate average
                    float sum = 0;
                    for (int i = 0; i < valid_readings; i++) {
                        sum += temp_readings[i];
                    }
                    pacienteAtual.temperatura = sum / valid_readings;
                    printf("[TEMP] ✅ Temperatura real: %.1f°C (%d leituras)\n", 
                           pacienteAtual.temperatura, valid_readings);
                    goto temp_complete;
                }
            }
            
            // Fallback to simulation
            printf("[TEMP] 🎯 Usando simulação de temperatura\n");
            
            task_command_t ui_cmd;
            ui_cmd.cmd = UI_CMD_UPDATE_TEXT;
            ui_cmd.param1 = 0;
            strcpy(ui_cmd.text, "Simulando medicao...");
            xQueueSend(xUICommandQueue, &ui_cmd, 0);
            
            uint32_t sim_start = to_ms_since_boot(get_absolute_time());
            
            while (estado_atual == ESTADO_TEMP && 
                   (to_ms_since_boot(get_absolute_time()) - sim_start) < 3000) {
                
                uint32_t current_time = to_ms_since_boot(get_absolute_time());
                uint32_t elapsed = current_time - sim_start;
                int progress = (elapsed * 100) / 3000;
                
                // Update status text (sem acentos)
                ui_cmd.cmd = UI_CMD_UPDATE_TEXT;
                ui_cmd.param1 = 0;
                sprintf(ui_cmd.text, "Simulando... %d%%", progress);
                xQueueSend(xUICommandQueue, &ui_cmd, 0);
                
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            
            // Generate realistic simulated temperature
            float base_temp = 36.0f + ((float)(rand() % 250)) / 100.0f; // 36.0 - 38.5°C
            pacienteAtual.temperatura = base_temp;
            
            printf("[TEMP] ✅ Simulação concluída\n");

        temp_complete:
            // Update final status (sem acentos)
            task_command_t final_cmd = {
                .cmd = UI_CMD_UPDATE_TEXT,
                .param1 = 0
            };
            strcpy(final_cmd.text, "Medicao concluida!");
            xQueueSend(xUICommandQueue, &final_cmd, 0);
            
            if (pacienteAtual.distancia_mm == 0) {
                pacienteAtual.distancia_mm = 40; // Default distance if not measured
            }
            pacienteAtual.valid = true;
            
            const char *status = (pacienteAtual.temperatura >= 37.8f) ? "FEBRE" :
                                (pacienteAtual.temperatura >= 37.5f) ? "Alerta" : "Normal";
            
            printf("TEMP COMPLETA -> %.1f°C -> %s\n", pacienteAtual.temperatura, status);
            
            xEventGroupSetBits(xEvents, EVT_TEMP_READY);
            
            while (estado_atual == ESTADO_TEMP)
            {
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
