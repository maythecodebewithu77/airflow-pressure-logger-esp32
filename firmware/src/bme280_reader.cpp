#include "bme280_reader.h"
#include <math.h>

Bme280Reader::Bme280Reader()
    : m_initialized(false),
      m_demo_mode(DEMO_MODE),
      m_baseline_temp_c(25.0f) {
    m_data.timestamp_ms = 0;
    m_data.temperature_c = 25.0f;
    m_data.humidity_percent = 50.0f;
    m_data.pressure_pa = 101325.0f;
    m_data.is_valid = false;
    m_data.thermal_drift_warning = false;
}

bool Bme280Reader::begin(TwoWire &wire) {
    if (m_demo_mode) {
        m_initialized = true;
        m_data.is_valid = true;
        return true;
    }

    if (!m_bme.begin(BME280_I2C_ADDR, &wire)) {
        m_initialized = false;
        m_data.is_valid = false;
        return false;
    }

    // Set sensor to forced / weather monitoring profile (low rate, low power)
    m_bme.setSampling(Adafruit_BME280::MODE_FORCED,
                      Adafruit_BME280::SAMPLING_X1, // Temperature oversampling
                      Adafruit_BME280::SAMPLING_X1, // Pressure oversampling
                      Adafruit_BME280::SAMPLING_X1, // Humidity oversampling
                      Adafruit_BME280::FILTER_OFF);

    m_initialized = true;
    m_data.is_valid = true;
    update();
    latchBaselineTemperature();
    return true;
}

void Bme280Reader::latchBaselineTemperature() {
    if (m_data.is_valid) {
        m_baseline_temp_c = m_data.temperature_c;
    }
}

Bme280Data Bme280Reader::update() {
    uint32_t now = millis();
    m_data.timestamp_ms = now;

    if (m_demo_mode) {
        // Simulated slow room variations
        m_data.temperature_c = 25.4f + 0.3f * sinf(now * 0.0001f);
        m_data.humidity_percent = 58.2f + 1.5f * cosf(now * 0.0001f);
        m_data.pressure_pa = 101310.0f + 15.0f * sinf(now * 0.00005f);
        m_data.is_valid = true;
        m_data.thermal_drift_warning = false;
        return m_data;
    }

    if (!m_initialized) {
        m_data.is_valid = false;
        return m_data;
    }

    // Trigger measurement in forced mode
    m_bme.takeForcedMeasurement();

    m_data.temperature_c = m_bme.readTemperature();
    m_data.humidity_percent = m_bme.readHumidity();
    m_data.pressure_pa = m_bme.readPressure(); // Pascals
    m_data.is_valid = true;

    // Check for thermal drift from the moment zero-calibration was performed
    if (fabsf(m_data.temperature_c - m_baseline_temp_c) > 2.0f) {
        m_data.thermal_drift_warning = true;
    } else {
        m_data.thermal_drift_warning = false;
    }

    return m_data;
}
