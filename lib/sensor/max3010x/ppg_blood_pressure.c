#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "ppg_blood_pressure.h"


void normalize_ppg_buffer(float *buffer, int length)
{
    float min_val = buffer[0], max_val = buffer[0];
    for (int i = 1; i < length; i++)
    {
        if (buffer[i] < min_val)
            min_val = buffer[i];
        if (buffer[i] > max_val)
            max_val = buffer[i];
    }

    float range = max_val - min_val;
    if (range > 0)
    {
        for (int i = 0; i < length; i++)
        {
            buffer[i] = ((buffer[i] - min_val) / range) * 10000.0f;
        }
    }
}

ppg_features_t extract_ppg_features(const float *ir_data, const float *red_data, int length)
{
    ppg_features_t features = {0};

    if (length < 50) return features;

    // Usar alocação dinâmica para o buffer de suavização para economizar BSS
    float *smoothed = (float*)malloc(length * sizeof(float));
    if (smoothed == NULL) {
        printf("[PPG] Falha ao alocar memória para suavização\n");
        return features;
    }

    // Suavização simples
    for (int i = 0; i < length; i++) {
        if (i == 0) {
            smoothed[i] = ir_data[i];
        } else if (i == length - 1) {
            smoothed[i] = ir_data[i];
        } else {
            smoothed[i] = (ir_data[i-1] + ir_data[i] + ir_data[i+1]) / 3.0f;
        }
    }

    // Pico/vale
    float max_val = -999999, min_val = 999999;
    int max_idx = 0, min_idx = 0;
    for (int i = 0; i < length; i++) {
        if (smoothed[i] > max_val) { max_val = smoothed[i]; max_idx = i; }
        if (smoothed[i] < min_val) { min_val = smoothed[i]; min_idx = i; }
    }
    features.systolic_peak = max_val;
    features.diastolic_valley = min_val;
    features.pulse_amplitude = max_val - min_val;

    // Tempo de subida 25%->75%
    float amp_25 = min_val + 0.25f * (max_val - min_val);
    float amp_75 = min_val + 0.75f * (max_val - min_val);
    int rise_start = -1, rise_end = -1;
    int search_start = (max_idx - 50 > 0) ? max_idx - 50 : 0;
    int search_end = max_idx;
    for (int i = search_start; i < search_end; i++) {
        if (rise_start == -1 && smoothed[i] >= amp_25) rise_start = i;
        if (rise_start != -1 && smoothed[i] >= amp_75) { rise_end = i; break; }
    }
    if (rise_start != -1 && rise_end != -1 && rise_end > rise_start) {
        features.rise_time = (rise_end - rise_start) * 10.0f; // ms
        if (features.rise_time < 10.0f) features.rise_time = 10.0f;
        if (features.rise_time > 200.0f) features.rise_time = 200.0f;
    } else {
        features.rise_time = 100.0f;
    }

    // Área sob a curva
    float auc = 0;
    for (int i = 0; i < length - 1; i++) {
        auc += (smoothed[i] + smoothed[i + 1]) / 2.0f;
    }
    features.area_under_curve = auc;

    // Energia espectral simples (derivada)
    float spectral_energy = 0;
    for (int i = 1; i < length - 1; i++) {
        float derivative = (smoothed[i + 1] - smoothed[i - 1]) / 2.0f;
        spectral_energy += derivative * derivative;
    }
    features.spectral_energy = spectral_energy / length;

    // Libera memória alocada
    free(smoothed);

    return features;
}

bp_estimate_t estimate_blood_pressure(ppg_features_t features, int heart_rate) {
    bp_estimate_t bp = {0};

    if (features.pulse_amplitude < 100 || heart_rate < 50 || heart_rate > 150) {
        bp.confidence = 0.0f;
        return bp;
    }

    float rise_time_clamped = features.rise_time;
    if (rise_time_clamped < 50.0f) rise_time_clamped = 50.0f;
    if (rise_time_clamped > 150.0f) rise_time_clamped = 150.0f;

    float baseline_map = 92.0f;
    float hr_factor = (heart_rate - 70.0f) * 0.2f;
    float rise_factor = (rise_time_clamped - 100.0f) * -0.15f;
    float estimated_map = baseline_map + hr_factor + rise_factor;

    float pulse_pressure = 28.0f + (features.pulse_amplitude / 10000.0f) * 8.0f;

    bp.systolic_bp = estimated_map + (pulse_pressure * 0.7f);
    bp.diastolic_bp = estimated_map - (pulse_pressure * 0.15f);

    if (bp.systolic_bp < 90) bp.systolic_bp = 90;
    if (bp.systolic_bp > 180) bp.systolic_bp = 180;
    if (bp.diastolic_bp < 60) bp.diastolic_bp = 60;
    if (bp.diastolic_bp > 110) bp.diastolic_bp = 110;

    float signal_quality = fminf(1.0f, features.pulse_amplitude / 5000.0f);
    float hr_quality = (heart_rate >= 60 && heart_rate <= 100) ? 1.0f : 0.8f;
    float rise_quality = (rise_time_clamped >= 50.0f && rise_time_clamped <= 150.0f) ? 1.0f : 0.6f;
    bp.confidence = signal_quality * hr_quality * rise_quality;

    if (bp.systolic_bp <= bp.diastolic_bp) {
        bp.confidence = 0.0f;
    }

    return bp;
}