#ifndef BREATHING_H
#define BREATHING_H

#include <Arduino.h>
#include "config.h"
#include "airflow.h"

enum BreathPhase {
    BREATH_PHASE_IDLE,
    BREATH_PHASE_INSPIRATION,
    BREATH_PHASE_EXPIRATION
};

struct BreathingMetrics {
    uint32_t timestamp_ms;
    float bpm;                      // Rolling Breaths Per Minute
    float peak_insp_flow_lmin;      // Peak inspiratory relative flow magnitude (rel. L/min)
    float peak_exp_flow_lmin;       // Peak expiratory relative flow magnitude (rel. L/min)
    float ie_ratio;                 // Inhale to Exhale time ratio (1 : X)
    float current_pause_duration_s; // Current pause duration if active (seconds)
    uint8_t status_flags;           // Bitmask: NORMAL, LOW_FLOW, PAUSE_ALERT, SENSOR_FAULT, DEMO
    bool pause_event_raised;        // True on the rising edge of a pause threshold crossing
    bool pause_event_completed;     // True when a breathing pause ends and normal breath resumes
    uint32_t last_pause_start_ms;   // Timestamp when last pause began
    uint32_t last_pause_duration_ms;// Total duration of last completed pause
};

class BreathingAnalyzer {
public:
    BreathingAnalyzer();

    // Process a new 50 Hz airflow sample and update metrics
    BreathingMetrics update(const AirflowSample &sample);

    // Reset breathing analysis state (e.g. after zero-calibration or disconnect)
    void reset();

    // Retrieve active metrics
    const BreathingMetrics& getMetrics() const { return m_metrics; }

private:
    BreathingMetrics m_metrics;
    BreathPhase m_current_phase;

    // Breath timing & rolling BPM buffer
    uint32_t m_last_breath_start_ms;
    uint32_t m_insp_start_ms;
    uint32_t m_exp_start_ms;
    float m_insp_duration_ms;
    float m_exp_duration_ms;

    uint32_t m_breath_intervals[BPM_ROLLING_BUFFER_SIZE];
    uint8_t m_buffer_count;
    uint8_t m_buffer_index;

    // Peak flow tracking in active cycle
    float m_cycle_peak_insp;
    float m_cycle_peak_exp;

    // Pause tracking
    uint32_t m_pause_timer_start_ms;
    bool m_pause_alert_latched;

    void updateBpm(uint32_t cycle_duration_ms);
};

#endif // BREATHING_H
