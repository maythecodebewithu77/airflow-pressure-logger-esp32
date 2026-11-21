#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "airflow.h"
#include "breathing.h"
#include "bme280_reader.h"
#include "battery.h"
#include "ble_service.h"
#include "logger.h"

// =============================================================================
// GLOBAL SUBSYSTEM INSTANCES
// =============================================================================
AirflowSensor         g_airflow_sensor;
BreathingAnalyzer     g_breathing_analyzer;
Bme280Reader          g_bme280;
BatteryMonitor        g_battery;
BleLoggerService      g_ble;
SerialTelemetryLogger g_logger;

// Task timing trackers
static uint32_t g_last_sample_ms   = 0;
static uint32_t g_last_ble_wave_ms = 0;
static uint32_t g_last_summary_ms  = 0;
static uint32_t g_last_battery_ms  = 0;

// Cached environmental and battery states
static Bme280Data   g_cached_env;
static BatteryState g_cached_bat;

void setup() {
    // 1. Initialize Status LED
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, HIGH); // Turn on during boot

    // 2. Initialize Serial Logging Console
    g_logger.begin(SERIAL_BAUD_RATE);
    g_logger.logEvent("BOOT", "Initializing Breathing Airflow and Mask-Pressure Logger...");

    // 3. Initialize Shared I2C Bus
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.setClock(400000); // 400 kHz Fast I2C
    g_logger.logEvent("I2C", "I2C bus initialized at 400kHz on SDA=21, SCL=22");

    // 4. Initialize Airflow Sensor (ADS1115 + MPXV7002DP)
    if (g_airflow_sensor.begin(Wire)) {
        g_logger.logEvent("AIRFLOW", "ADS1115 initialized. Zero-offset calibration started (3s calm air)...");
    } else {
        g_logger.logEvent("AIRFLOW", "WARNING: ADS1115 not detected! Operating in fault or simulated mode.");
    }

    // 5. Initialize Environmental Sensor (BME280)
    if (g_bme280.begin(Wire)) {
        g_logger.logEvent("BME280", "BME280 detected and initialized in forced mode.");
    } else {
        g_logger.logEvent("BME280", "WARNING: BME280 not found on 0x76. Environmental telemetry disabled.");
    }

    // 6. Initialize Battery Monitor
    g_battery.begin();
    g_cached_bat = g_battery.update();
    g_cached_env = g_bme280.update();

    // 7. Initialize BLE GATT Server
    g_logger.logEvent("BLE", "Starting Bluetooth Low Energy GATT Server...");
    g_ble.begin(BLE_DEVICE_NAME);
    g_logger.logEvent("BLE", "GATT Server advertising as: Airflow-Logger-ESP32");

    // Turn off boot LED indication
    digitalWrite(PIN_STATUS_LED, LOW);
    g_logger.logEvent("READY", "System initialized. Breathing analysis active.");
}

void loop() {
    uint32_t now = millis();

    // =========================================================================
    // TASK 1: 50 Hz Airflow Acquisition & Breathing DSP (~20 ms period)
    // =========================================================================
    if (now - g_last_sample_ms >= AIRFLOW_SAMPLE_INTERVAL_MS) {
        g_last_sample_ms = now;

        // Acquire new differential pressure sample
        AirflowSample sample = g_airflow_sensor.update();

        // Process breathing state machine & metrics
        BreathingMetrics metrics = g_breathing_analyzer.update(sample);

        // Inject compile-time demo flag if enabled
        if (g_airflow_sensor.isDemoMode()) {
            metrics.status_flags |= STATUS_BIT_DEMO_MODE;
        }

        // Handle breathing pause alert events
        if (metrics.pause_event_raised) {
            g_logger.logPauseAlert((uint32_t)(metrics.current_pause_duration_s * 1000.0f));
            g_ble.notifyPauseEvent(metrics.last_pause_start_ms, (uint32_t)(metrics.current_pause_duration_s * 1000.0f));
        } else if (metrics.pause_event_completed) {
            g_logger.logEvent("ALERT", "Breathing pause cleared. Normal flow resumed.");
            g_ble.notifyPauseEvent(metrics.last_pause_start_ms, metrics.last_pause_duration_ms);
        }

        // Output to Serial Plotter or CSV stream
        if (SERIAL_PLOTTER_MODE) {
            g_logger.logPlotterSample(sample, metrics);
        } else if (SERIAL_CSV_MODE) {
            g_logger.logCsvRow(sample, metrics, g_cached_env, g_cached_bat);
        }

        // Visual LED Indicator
        if (sample.sensor_fault) {
            // Fast flash (5 Hz) indicates hardware fault
            digitalWrite(PIN_STATUS_LED, ((now / 100) % 2) == 0 ? HIGH : LOW);
        } else if (metrics.status_flags & STATUS_BIT_PAUSE_ALERT) {
            // Steady ON indicates active breathing pause alert
            digitalWrite(PIN_STATUS_LED, HIGH);
        } else if (fabsf(sample.relative_flow_lmin) > BREATH_START_THRESHOLD_FLOW) {
            // Pulse ON during active breathing phase
            digitalWrite(PIN_STATUS_LED, HIGH);
        } else {
            digitalWrite(PIN_STATUS_LED, LOW);
        }
    }

    // =========================================================================
    // TASK 2: 20 Hz BLE Waveform Stream (~50 ms period)
    // =========================================================================
    if (now - g_last_ble_wave_ms >= BLE_WAVEFORM_NOTIFY_INTERVAL_MS) {
        g_last_ble_wave_ms = now;
        if (g_ble.isConnected()) {
            AirflowSample sample;
            sample.timestamp_ms = now;
            sample.clean_pressure_pa = g_airflow_sensor.update().clean_pressure_pa;
            sample.relative_flow_lmin = g_airflow_sensor.update().relative_flow_lmin;
            g_ble.notifyWaveform(sample);
        }
    }

    // =========================================================================
    // TASK 3: 1 Hz Summary Metrics & Environmental Acquisition (~1000 ms)
    // =========================================================================
    if (now - g_last_summary_ms >= SUMMARY_NOTIFY_INTERVAL_MS) {
        g_last_summary_ms = now;

        // Sample BME280 in forced mode
        g_cached_env = g_bme280.update();

        if (g_ble.isConnected()) {
            BreathingMetrics metrics = g_breathing_analyzer.getMetrics();
            if (g_airflow_sensor.isDemoMode()) {
                metrics.status_flags |= STATUS_BIT_DEMO_MODE;
            }
            g_ble.notifyMetrics(metrics);
            g_ble.notifyEnvironment(g_cached_env);
        }
    }

    // =========================================================================
    // TASK 4: 0.5 Hz Battery Monitor (~2000 ms period)
    // =========================================================================
    if (now - g_last_battery_ms >= BATTERY_SAMPLE_INTERVAL_MS) {
        g_last_battery_ms = now;
        g_cached_bat = g_battery.update();

        if (g_ble.isConnected()) {
            g_ble.notifyBattery(g_cached_bat);
        }
    }

    // Yield CPU time to ESP32 RTOS background tasks & BLE stack
    yield();
}
