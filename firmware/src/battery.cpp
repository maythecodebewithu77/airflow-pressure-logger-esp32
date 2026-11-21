#include "battery.h"
#include <math.h>

BatteryMonitor::BatteryMonitor()
    : m_demo_mode(DEMO_MODE),
      m_filtered_voltage(3.85f) {
    m_state.timestamp_ms = 0;
    m_state.voltage_volts = 3.85f;
    m_state.percentage = 80;
    m_state.is_low_battery = false;
    m_state.is_charging = false;
}

void BatteryMonitor::begin() {
    // Configure ESP32 ADC pin
    pinMode(PIN_BATTERY_ADC, INPUT);
    analogSetPinAttenuation(PIN_BATTERY_ADC, ADC_11db); // 0 to 3.1V range
    update();
}

uint8_t BatteryMonitor::estimateSoc(float volts) {
    // Piecewise linear model for 1S LiPo (3.7V nominal)
    if (volts >= 4.15f) return 100;
    if (volts >= 4.05f) return 90 + (uint8_t)((volts - 4.05f) / 0.10f * 10);
    if (volts >= 3.90f) return 70 + (uint8_t)((volts - 3.90f) / 0.15f * 20);
    if (volts >= 3.75f) return 40 + (uint8_t)((volts - 3.75f) / 0.15f * 30);
    if (volts >= 3.65f) return 20 + (uint8_t)((volts - 3.65f) / 0.10f * 20);
    if (volts >= 3.40f) return 5  + (uint8_t)((volts - 3.40f) / 0.25f * 15);
    return 0;
}

BatteryState BatteryMonitor::update() {
    uint32_t now = millis();
    m_state.timestamp_ms = now;

    if (m_demo_mode) {
        // Simulated slow battery drain
        m_state.voltage_volts = 3.92f - 0.000005f * (float)now;
        m_state.percentage = estimateSoc(m_state.voltage_volts);
        m_state.is_low_battery = (m_state.percentage <= BAT_LOW_PERCENT_THRESHOLD);
        m_state.is_charging = false;
        return m_state;
    }

    // Read ADC with 16-sample averaging to suppress noise
    uint32_t adc_accum = 0;
    for (int i = 0; i < 16; i++) {
        adc_accum += analogRead(PIN_BATTERY_ADC);
        delayMicroseconds(50);
    }
    float raw_adc_avg = (float)adc_accum / 16.0f;

    // ESP32 ADC at 11dB attenuation: ~3.3V full scale for 4095 counts
    float pin_voltage = (raw_adc_avg / 4095.0f) * 3.30f;

    // Undo 100k / 100k divider (multiply by 2.0)
    float cell_voltage = pin_voltage / BAT_DIVIDER_RATIO;

    // Exponential smoothing
    m_filtered_voltage = (0.2f * cell_voltage) + (0.8f * m_filtered_voltage);
    m_state.voltage_volts = m_filtered_voltage;
    m_state.percentage = estimateSoc(m_filtered_voltage);
    m_state.is_low_battery = (m_state.percentage <= BAT_LOW_PERCENT_THRESHOLD);
    m_state.is_charging = (m_filtered_voltage > 4.25f);

    return m_state;
}
