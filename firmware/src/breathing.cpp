#include "breathing.h"
#include <math.h>

BreathingAnalyzer::BreathingAnalyzer() {
    reset();
}

void BreathingAnalyzer::reset() {
    m_metrics.timestamp_ms = 0;
    m_metrics.bpm = 0.0f;
    m_metrics.peak_insp_flow_lmin = 0.0f;
    m_metrics.peak_exp_flow_lmin = 0.0f;
    m_metrics.ie_ratio = 1.0f;
    m_metrics.current_pause_duration_s = 0.0f;
    m_metrics.status_flags = STATUS_BIT_NORMAL;
    m_metrics.pause_event_raised = false;
    m_metrics.pause_event_completed = false;
    m_metrics.last_pause_start_ms = 0;
    m_metrics.last_pause_duration_ms = 0;

    m_current_phase = BREATH_PHASE_IDLE;
    m_last_breath_start_ms = 0;
    m_insp_start_ms = 0;
    m_exp_start_ms = 0;
    m_insp_duration_ms = 1500.0f;
    m_exp_duration_ms = 2500.0f;

    m_buffer_count = 0;
    m_buffer_index = 0;
    for (uint8_t i = 0; i < BPM_ROLLING_BUFFER_SIZE; i++) {
        m_breath_intervals[i] = 0;
    }

    m_cycle_peak_insp = 0.0f;
    m_cycle_peak_exp = 0.0f;
    m_pause_timer_start_ms = 0;
    m_pause_alert_latched = false;
}

void BreathingAnalyzer::updateBpm(uint32_t cycle_duration_ms) {
    if (cycle_duration_ms < MIN_BREATH_PERIOD_MS || cycle_duration_ms > MAX_BREATH_PERIOD_MS) {
        return; // Ignore spurious or out-of-bounds breath periods
    }

    m_breath_intervals[m_buffer_index] = cycle_duration_ms;
    m_buffer_index = (m_buffer_index + 1) % BPM_ROLLING_BUFFER_SIZE;
    if (m_buffer_count < BPM_ROLLING_BUFFER_SIZE) {
        m_buffer_count++;
    }

    // Compute rolling average period
    uint32_t sum = 0;
    for (uint8_t i = 0; i < m_buffer_count; i++) {
        sum += m_breath_intervals[i];
    }
    float avg_period_sec = ((float)sum / (float)m_buffer_count) / 1000.0f;
    if (avg_period_sec > 0.0f) {
        m_metrics.bpm = 60.0f / avg_period_sec;
    }
}

BreathingMetrics BreathingAnalyzer::update(const AirflowSample &sample) {
    uint32_t now = sample.timestamp_ms;
    m_metrics.timestamp_ms = now;
    m_metrics.pause_event_raised = false;
    m_metrics.pause_event_completed = false;

    // -------------------------------------------------------------------------
    // 1. Hardware Integrity Check
    // A disconnected or flatlined sensor MUST NEVER be reported as a breathing pause.
    // -------------------------------------------------------------------------
    if (sample.sensor_fault) {
        m_metrics.status_flags = STATUS_BIT_SENSOR_FAULT;
        m_pause_timer_start_ms = 0;
        m_pause_alert_latched = false;
        m_metrics.current_pause_duration_s = 0.0f;
        m_current_phase = BREATH_PHASE_IDLE;
        return m_metrics;
    }

    float flow = sample.relative_flow_lmin;
    float abs_flow = fabsf(flow);

    // -------------------------------------------------------------------------
    // 2. Breathing Pause Alert (Prototype) Logic
    // -------------------------------------------------------------------------
    if (abs_flow < PAUSE_THRESHOLD_FLOW) {
        if (m_pause_timer_start_ms == 0) {
            m_pause_timer_start_ms = now;
        }
        float pause_sec = (float)(now - m_pause_timer_start_ms) / 1000.0f;
        m_metrics.current_pause_duration_s = pause_sec;

        if (pause_sec >= PAUSE_ALERT_THRESHOLD_SEC) {
            if (!m_pause_alert_latched) {
                // Rising edge of pause threshold crossing
                m_pause_alert_latched = true;
                m_metrics.pause_event_raised = true;
                m_metrics.last_pause_start_ms = m_pause_timer_start_ms;
            }
            m_metrics.status_flags |= STATUS_BIT_PAUSE_ALERT;
        }
    } else if (abs_flow >= BREATH_START_THRESHOLD_FLOW) {
        // Active breath resumed
        if (m_pause_alert_latched) {
            // Falling edge: pause completed
            m_metrics.last_pause_duration_ms = now - m_metrics.last_pause_start_ms;
            m_metrics.pause_event_completed = true;
            m_pause_alert_latched = false;
        }
        m_pause_timer_start_ms = 0;
        m_metrics.current_pause_duration_s = 0.0f;
        m_metrics.status_flags &= ~STATUS_BIT_PAUSE_ALERT;
    }

    // -------------------------------------------------------------------------
    // 3. Inhale / Exhale State Machine & Phase Detection
    // Negative flow = Inspiration; Positive flow = Expiration
    // -------------------------------------------------------------------------
    if (flow < -BREATH_START_THRESHOLD_FLOW) {
        // Entering or continuing Inspiration
        if (m_current_phase != BREATH_PHASE_INSPIRATION) {
            m_current_phase = BREATH_PHASE_INSPIRATION;
            m_insp_start_ms = now;

            // Finalize expiration duration if valid
            if (m_exp_start_ms > 0) {
                m_exp_duration_ms = (float)(now - m_exp_start_ms);
            }

            // Calculate cycle period from previous breath start
            if (m_last_breath_start_ms > 0) {
                updateBpm(now - m_last_breath_start_ms);
            }
            m_last_breath_start_ms = now;

            // Latch previous cycle peaks and reset tracking
            m_metrics.peak_insp_flow_lmin = m_cycle_peak_insp;
            m_metrics.peak_exp_flow_lmin = m_cycle_peak_exp;
            m_cycle_peak_insp = 0.0f;
            m_cycle_peak_exp = 0.0f;

            // Update I:E ratio
            if (m_insp_duration_ms > 100.0f) {
                m_metrics.ie_ratio = m_exp_duration_ms / m_insp_duration_ms;
            }
        }

        // Track peak inspiratory flow
        if (abs_flow > m_cycle_peak_insp) {
            m_cycle_peak_insp = abs_flow;
        }

    } else if (flow > BREATH_START_THRESHOLD_FLOW) {
        // Entering or continuing Expiration
        if (m_current_phase != BREATH_PHASE_EXPIRATION) {
            m_current_phase = BREATH_PHASE_EXPIRATION;
            m_exp_start_ms = now;

            if (m_insp_start_ms > 0) {
                m_insp_duration_ms = (float)(now - m_insp_start_ms);
            }
        }

        // Track peak expiratory flow
        if (abs_flow > m_cycle_peak_exp) {
            m_cycle_peak_exp = abs_flow;
        }

    } else if (abs_flow < BREATH_END_HYSTERESIS_FLOW) {
        // Zero or inter-breath transition
        m_current_phase = BREATH_PHASE_IDLE;
    }

    // -------------------------------------------------------------------------
    // 4. Status Flag Assembly
    // -------------------------------------------------------------------------
    if (!(m_metrics.status_flags & STATUS_BIT_PAUSE_ALERT)) {
        if (m_metrics.peak_insp_flow_lmin > 0.0f && m_metrics.peak_insp_flow_lmin < LOW_FLOW_THRESHOLD_FLOW) {
            m_metrics.status_flags = STATUS_BIT_LOW_FLOW;
        } else {
            m_metrics.status_flags = STATUS_BIT_NORMAL;
        }
    }

    return m_metrics;
}
