#ifndef AIRFLOW_H
#define AIRFLOW_H

#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include "config.h"

struct AirflowSample {
    uint32_t timestamp_ms;
    float raw_voltage;        // Scaled voltage at ADS1115 A0 (Volts)
    float raw_pressure_pa;    // Instantaneous differential pressure (Pa)
    float filtered_pressure_pa;// Low-pass filtered differential pressure (Pa)
    float baseline_drift_pa;  // Tracked baseline drift (Pa)
    float clean_pressure_pa;   // Drift-compensated differential pressure (Pa)
    float relative_flow_lmin; // Estimated relative airflow (rel. L/min)
    bool sensor_fault;        // True if disconnected, out-of-bounds, or flatlined
};

class AirflowSensor {
public:
    AirflowSensor();

    // Initialize ADS1115 ADC and begin startup calibration
    bool begin(TwoWire &wire = Wire);

    // Sample sensor at 50 Hz, run IIR filter, drift tracker, and fault checks
    AirflowSample update();

    // Trigger an in-session zero-offset re-calibration (e.g. from web dashboard)
    void triggerZeroCalibration();

    // Check if zero calibration is currently in progress
    bool isCalibrating() const { return m_calibrating; }

    // Retrieve the active zero-offset voltage (scaled Volts at ADS1115 A0)
    float getZeroOffsetVolts() const { return m_v_zero_scaled; }

    // Force demo mode on or off at runtime
    void setDemoMode(bool enabled) { m_demo_mode = enabled; }
    bool isDemoMode() const { return m_demo_mode; }

private:
    Adafruit_ADS1115 m_ads;
    bool m_adc_initialized;
    bool m_demo_mode;

    // Calibration state
    bool m_calibrating;
    uint16_t m_calib_sample_index;
    float m_calib_accumulator;
    float m_v_zero_scaled;

    // Filter states
    float m_filtered_pressure;
    float m_baseline_drift;
    float m_last_raw_voltage;

    // Fault detection state
    uint16_t m_flatline_counter;
    bool m_sensor_fault;

    // Demo mode waveform generator state
    uint32_t m_demo_start_ms;
    float m_demo_phase;

    // Helper math
    float calculateFlowFromPressure(float delta_p_pa);
    AirflowSample generateDemoSample(uint32_t now);
};

#endif // AIRFLOW_H
