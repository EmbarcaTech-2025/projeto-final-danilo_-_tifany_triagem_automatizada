#ifndef PPG_BLOOD_PRESSURE_H
#define PPG_BLOOD_PRESSURE_H

#ifdef __cplusplus
extern "C" {
#endif

// Standard includes
#include <stdint.h>
#include <stdbool.h>

// ==================== CONSTANTS ====================
#define RATE_SIZE 4
#define FINGER_THRESHOLD 200000
#define BEAT_TIMEOUT_MS 10000
#define STABILIZATION_TIME_MS 10000
#define PPG_BUFFER_SIZE 50  // Reduced for memory optimization

// ==================== TYPE DEFINITIONS ====================

/**
 * @brief PPG signal features extracted for blood pressure estimation
 */
typedef struct {
    float systolic_peak;           /**< Peak value during systole */
    float diastolic_valley;        /**< Valley value during diastole */
    float pulse_amplitude;         /**< Amplitude of pulse wave */
    float rise_time;              /**< Rise time of pulse wave (ms) */
    float fall_time;              /**< Fall time of pulse wave (ms) */
    float area_under_curve;       /**< Area under the pulse curve */
    float heart_rate_variability; /**< Heart rate variability measure */
    float spectral_energy;        /**< Spectral energy of the signal */
} ppg_features_t;

/**
 * @brief Blood pressure estimation result
 */
typedef struct {
    float systolic_bp;    /**< Estimated systolic blood pressure (mmHg) */
    float diastolic_bp;   /**< Estimated diastolic blood pressure (mmHg) */
    float confidence;     /**< Confidence level (0.0 to 1.0) */
} bp_estimate_t;

// ==================== FUNCTION DECLARATIONS ====================

/**
 * @brief Normalize PPG buffer data to a consistent scale
 * @param buffer Pointer to the data buffer to normalize
 * @param length Number of samples in the buffer
 */
void normalize_ppg_buffer(float *buffer, int length);

/**
 * @brief Extract relevant features from PPG signal data
 * @param ir_data Infrared LED data buffer
 * @param red_data Red LED data buffer  
 * @param length Number of samples in each buffer
 * @return ppg_features_t Structure containing extracted features
 */
ppg_features_t extract_ppg_features(const float *ir_data, const float *red_data, int length);

/**
 * @brief Estimate blood pressure from PPG features and heart rate
 * @param features PPG signal features
 * @param heart_rate Current heart rate in BPM
 * @return bp_estimate_t Blood pressure estimation with confidence
 */
bp_estimate_t estimate_blood_pressure(ppg_features_t features, int heart_rate);

#ifdef __cplusplus
}
#endif

#endif /* PPG_BLOOD_PRESSURE_H */