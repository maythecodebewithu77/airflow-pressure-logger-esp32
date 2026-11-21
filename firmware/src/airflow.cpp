#include "airflow.h"
#include <math.h>

AirflowSensor::AirflowSensor()
    : m_adc_initialized(false),
      m_demo_mode(DEMO_MODE),
      m_calibrating(false),
      m_calib_sample_index(0),
      m_calib_accumulator(0.0f),
      m_v_zero_scaled(MPX_NOMINAL_ZERO_VOLTS * SENSOR_VOLTAGE_DIVIDER_RATIO),
      m_filtered_pressure(0.0f),
      m_baseline_drift(0.0f),
      m_last_raw_voltage(0.0f),
      m_flatline_counter(0),
      m_sensor_fault(false),
      m_demo_start_ms(0),
      m_demo_phase(0.0f) {}

bool AirflowSensor::begin(TwoWire &wire) {
    if (m_demo_mode) {
        m_adc_initialized = true;
        m_demo_start_ms = millis();
        return true;
    }

    // Initialize ADS1115 with custom I2C pins
    wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    if (!m_ads.begin(ADS1115_I2C_ADDR, &wire)) {
        m_sensor_fault = true;
        m_adc_initialized = false;
        return false;
    }

    // Set gain to +-4.096V (PGA = 1) -> 0.125 mV per LSB
    m_ads.setGain(GAIN_ONE);
    m_adc_initialized = true;

    // Start initial zero-offset calibration
    triggerZeroCalibration();
    return true;
}

void AirflowSensor::triggerZeroCalibration() {
    m_calibrating = true;
    m_calib_sample_index = 0;
    m_calib_accumulator = 0.0f;
}

float AirflowSensor::calculateFlowFromPressure(float delta_p_pa) {
    // Flow formula across orifice: Q = k * sgn(dP) * sqrt(|dP|)
    // Note: Numbers are relative estimates until calibrated against a known standard.
    if (fabsf(delta_p_pa) < 0.05f) {
        return 0.0f;
    }
    float sign = (delta_p_pa >= 0.0f) ? 1.0f : -1.0f;
    return sign * FLOW_CALIBRATION_COEFF * sqrtf(fabsf(delta_p_pa));
}

AirflowSample AirflowSensor::generateDemoSample(uint32_t now) {
    AirflowSample sample;
    sample.timestamp_ms = now;
    sample.sensor_fault = false;

    // Generate simulated breathing pattern (~15 BPM, period = 4.0s)
    // Breathing cycle: Inhalation (negative, 1.6s) -> Exhalation (positive, 2.4s)
    // Every 45 seconds, inject a simulated 12-second breath pause.
    uint32_t cycle_time_ms = (now - m_demo_start_ms) % 45000;

    if (cycle_time_ms > 25000 && cycle_time_ms < 37000) {
        // Simulated breathing pause (12 seconds)
        sample.raw_voltage = m_v_zero_scaled + 0.0005f * sinf(now * 0.005f);
        sample.raw_pressure_pa = 0.0f;
        sample.filtered_pressure_pa = 0.0f;
        sample.baseline_drift_pa = 0.0f;
        sample.clean_pressure_pa = 0.0f;
        sample.relative_flow_lmin = 0.0f;
    } else {
        // Normal synthetic breathing pattern
        float t_sec = (float)(now % 4000) / 4000.0f; // 4 second period = 15 BPM
        float flow_sim = 0.0f;

        if (t_sec < 0.40f) {
            // Inhalation (negative flow, peak -24 rel. L/min)
            flow_sim = -24.0f * sinf(t_sec / 0.40f * 3.14159f);
        } else {
            // Exhalation (positive flow, peak +20 rel. L/min)
            flow_sim = 20.0f * sinf((t_sec - 0.40f) / 0.60f * 3.14159f);
        }

        // Add subtle pseudo-turbulence noise
        float noise = ((float)(rand() % 100) / 100.0f - 0.5f) * 1.2f;
        sample.relative_flow_lmin = flow_sim + noise;

        // Invert flow relationship to produce pressure: dP = sgn(Q) * (Q / k)^2
        float sign = (sample.relative_flow_lmin >= 0.0f) ? 1.0f : -1.0f;
        float dP = sign * powf(sample.relative_flow_lmin / FLOW_CALIBRATION_COEFF, 2.0f);

        sample.clean_pressure_pa = dP;
        sample.filtered_pressure_pa = dP;
        sample.baseline_drift_pa = 0.0f;
        sample.raw_pressure_pa = dP + ((float)(rand() % 100) / 100.0f - 0.5f) * 3.0f;

        // Convert back to simulated ADC voltage
        float v_diff_unscaled = (dP / 1000.0f) * MPX_SENSITIVITY_V_PER_KPA;
        float v_sensor = MPX_NOMINAL_ZERO_VOLTS + v_diff_unscaled;
        sample.raw_voltage = v_sensor * SENSOR_VOLTAGE_DIVIDER_RATIO;
    }

    return sample;
}

AirflowSample AirflowSensor::update() {
    uint32_t now = millis();

    if (m_demo_mode) {
        return generateDemoSample(now);
    }

    AirflowSample sample;
    sample.timestamp_ms = now;
    sample.sensor_fault = false;

    if (!m_adc_initialized) {
        sample.raw_voltage = 0.0f;
        sample.raw_pressure_pa = 0.0f;
        sample.filtered_pressure_pa = 0.0f;
        sample.baseline_drift_pa = 0.0f;
        sample.clean_pressure_pa = 0.0f;
        sample.relative_flow_lmin = 0.0f;
        sample.sensor_fault = true;
        return sample;
    }

    // Read single-ended voltage on ADS1115 Channel 0
    int16_t adc_counts = m_ads.readADC_SingleEnded(0);
    float v_scaled = adc_counts * ADS1115_VOLTS_PER_LSB;
    sample.raw_voltage = v_scaled;

    // -------------------------------------------------------------------------
    // 1. Fault Detection: Voltage Bounds & Flatline Detection
    // -------------------------------------------------------------------------
    if (v_scaled < ADC_VOLTAGE_FAULT_MIN || v_scaled > ADC_VOLTAGE_FAULT_MAX) {
        m_sensor_fault = true;
    } else {
        // Check for flatline (frozen / disconnected ADC pin)
        if (fabsf(v_scaled - m_last_raw_voltage) < SENSOR_FLATLINE_EPSILON_VOLTS) {
            m_flatline_counter++;
            if (m_flatline_counter >= SENSOR_FLATLINE_SAMPLE_COUNT) {
                m_sensor_fault = true;
            }
        } else {
            m_flatline_counter = 0;
            m_sensor_fault = false;
        }
    }
    m_last_raw_voltage = v_scaled;
    sample.sensor_fault = m_sensor_fault;

    // -------------------------------------------------------------------------
    // 2. Zero-Offset Calibration Routine
    // -------------------------------------------------------------------------
    if (m_calibrating) {
        m_calib_accumulator += v_scaled;
        m_calib_sample_index++;

        if (m_calib_sample_index >= ZERO_CALIBRATION_SAMPLES) {
            m_v_zero_scaled = m_calib_accumulator / (float)ZERO_CALIBRATION_SAMPLES;
            m_calibrating = false;
            m_filtered_pressure = 0.0f;
            m_baseline_drift = 0.0f;
        }

        // Return zeros while calibrating to suppress spurious breath detection
        sample.raw_pressure_pa = 0.0f;
        sample.filtered_pressure_pa = 0.0f;
        sample.baseline_drift_pa = 0.0f;
        sample.clean_pressure_pa = 0.0f;
        sample.relative_flow_lmin = 0.0f;
        return sample;
    }

    // If hardware fault detected, zero the physical metrics to avoid false alerts
    if (m_sensor_fault) {
        sample.raw_pressure_pa = 0.0f;
        sample.filtered_pressure_pa = 0.0f;
        sample.baseline_drift_pa = 0.0f;
        sample.clean_pressure_pa = 0.0f;
        sample.relative_flow_lmin = 0.0f;
        return sample;
    }

    // -------------------------------------------------------------------------
    // 3. Pressure Calculation & Signal Conditioning
    // -------------------------------------------------------------------------
    // Reverse divider scaling to obtain true sensor pin voltage
    float v_sensor = v_scaled / SENSOR_VOLTAGE_DIVIDER_RATIO;
    float v_zero_unscaled = m_v_zero_scaled / SENSOR_VOLTAGE_DIVIDER_RATIO;
    float v_diff = v_sensor - v_zero_unscaled;

    // MPXV7002DP: 1.0 V per kPa => 1.0 mV per Pa
    float raw_pressure_pa = (v_diff / MPX_SENSITIVITY_V_PER_KPA) * 1000.0f;
    sample.raw_pressure_pa = raw_pressure_pa;

    // Single-pole IIR low-pass filter (fc = 4.0 Hz at fs = 50 Hz)
    m_filtered_pressure = (FILTER_LPF_ALPHA * raw_pressure_pa) + ((1.0f - FILTER_LPF_ALPHA) * m_filtered_pressure);
    sample.filtered_pressure_pa = m_filtered_pressure;

    // Slow baseline drift tracker (compensates for room thermal & orientation changes)
    m_baseline_drift = (BASELINE_TRACKER_ALPHA * m_filtered_pressure) + ((1.0f - BASELINE_TRACKER_ALPHA) * m_baseline_drift);
    sample.baseline_drift_pa = m_baseline_drift;

    // Clean, zero-centered pressure
    float clean_pressure = m_filtered_pressure - m_baseline_drift;
    sample.clean_pressure_pa = clean_pressure;

    // Convert pressure to relative airflow (rel. L/min)
    sample.relative_flow_lmin = calculateFlowFromPressure(clean_pressure);

    return sample;
}
