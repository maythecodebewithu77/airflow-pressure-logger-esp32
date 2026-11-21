#ifndef BME280_READER_H
#define BME280_READER_H

#include <Arduino.h>
#include <Adafruit_BME280.h>
#include "config.h"

struct Bme280Data {
    uint32_t timestamp_ms;
    float temperature_c;       // Ambient/mask temperature in °C
    float humidity_percent;    // Relative humidity in %
    float pressure_pa;         // Absolute barometric pressure in Pa
    bool is_valid;             // True if sensor is communicating
    bool thermal_drift_warning;// True if temperature has shifted > 2.0°C from zeroing baseline
};

class Bme280Reader {
public:
    Bme280Reader();

    // Initialize sensor on I2C bus
    bool begin(TwoWire &wire = Wire);

    // Read sensor metrics (called at ~1 Hz)
    Bme280Data update();

    // Store baseline temperature at zero-calibration point
    void latchBaselineTemperature();

    // Set demo mode
    void setDemoMode(bool enabled) { m_demo_mode = enabled; }

    const Bme280Data& getData() const { return m_data; }

private:
    Adafruit_BME280 m_bme;
    Bme280Data m_data;
    bool m_initialized;
    bool m_demo_mode;
    float m_baseline_temp_c;
};

#endif // BME280_READER_H
