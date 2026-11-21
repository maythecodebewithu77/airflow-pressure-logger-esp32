#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// =============================================================================
// SYSTEM MODES & COMPILE-TIME FLAGS
// =============================================================================
// Set DEMO_MODE to 1 to generate synthetic breathing waveforms with pauses
// for UI testing without physical sensors connected. Set to 0 for real sensors.
#define DEMO_MODE 0

// Set to 1 to output Arduino Serial Plotter compatible format:
// "Raw_dP:%.1f,Filtered_dP:%.1f,Flow:%.1f,Baseline:%.1f"
#define SERIAL_PLOTTER_MODE 0

// Set to 1 to output structured CSV lines over UART for terminal logging
#define SERIAL_CSV_MODE 1

// Serial baud rate for USB console
#define SERIAL_BAUD_RATE 115200

// =============================================================================
// HARDWARE PIN DEFINITIONS (ESP32-WROOM-32)
// =============================================================================
#define PIN_I2C_SDA         21
#define PIN_I2C_SCL         22
#define PIN_BATTERY_ADC     34   // ADC1_CH6 (Input Only, safe with Wi-Fi/BLE)
#define PIN_STATUS_LED       2   // Onboard Blue LED

// =============================================================================
// I2C ADDRESSES & HARDWARE SETTINGS
// =============================================================================
#define ADS1115_I2C_ADDR    0x48 // ADDR pin tied to GND
#define BME280_I2C_ADDR     0x76 // SDO pin tied to GND (0x77 if tied to VDD)

// =============================================================================
// TASK & SAMPLING RATES
// =============================================================================
#define AIRFLOW_SAMPLE_RATE_HZ         50
#define AIRFLOW_SAMPLE_INTERVAL_MS     (1000 / AIRFLOW_SAMPLE_RATE_HZ) // 20 ms
#define BLE_WAVEFORM_NOTIFY_INTERVAL_MS 50                              // 20 Hz
#define BME280_SAMPLE_INTERVAL_MS      1000                            // 1 Hz
#define BATTERY_SAMPLE_INTERVAL_MS     2000                            // 0.5 Hz
#define SUMMARY_NOTIFY_INTERVAL_MS     1000                            // 1 Hz

// =============================================================================
// AIRFLOW SENSOR SIGNAL CONDITIONING (MPXV7002DP + ADS1115)
// =============================================================================
// MPXV7002DP: bidirectional +-2.0 kPa (+-2000 Pa)
// Nominal Vout = 5.0V * (0.2 * dP_kPa + 0.5) => Center = 2.50V, Sensitivity = 1.0 V/kPa
// Scaled via 10k / 15k divider (Factor = 15 / (10 + 15) = 0.60)
// Scaled Center = 1.50V, Scaled Span = 0.30V to 2.70V
#define MPX_SUPPLY_VOLTS              5.0f
#define MPX_NOMINAL_ZERO_VOLTS        2.50f
#define MPX_SENSITIVITY_V_PER_KPA     1.00f   // 1.0 V per kPa
#define SENSOR_VOLTAGE_DIVIDER_RATIO  0.60f   // 15k / (10k + 15k)

// ADS1115 full-scale gain setting (+-4.096V range, 1 LSB = 0.125 mV)
#define ADS1115_VOLTS_PER_LSB         0.000125f

// Low-pass filter coefficient (fc ~ 4.0 Hz at fs = 50 Hz)
// alpha = 2*pi*fc*dt / (1 + 2*pi*fc*dt) ~= 0.335
#define FILTER_LPF_ALPHA              0.335f

// Baseline drift tracking filter (slow EMA, tau ~ 15-20 s)
#define BASELINE_TRACKER_ALPHA        0.002f

// Flow calibration coefficient: Q = k * sgn(dP) * sqrt(|dP|)
// NOTE: Uncalibrated relative estimate. True volumetric accuracy requires
// bench calibration against a known reference syringe or flowmeter.
#define FLOW_CALIBRATION_COEFF        14.2f

// Zero calibration sample count during startup stabilization
#define ZERO_CALIBRATION_SAMPLES      150     // 3 seconds at 50 Hz

// =============================================================================
// FAULT & HARDWARE INTEGRITY THRESHOLDS
// =============================================================================
// Valid scaled voltage bounds at ADS1115 A0 (nominal center 1.50V, span 0.3V - 2.7V)
#define ADC_VOLTAGE_FAULT_MIN         0.15f   // Line shorted to GND or disconnected
#define ADC_VOLTAGE_FAULT_MAX         2.95f   // Line pulled to 3.3V rail
#define SENSOR_FLATLINE_SAMPLE_COUNT  100     // 2.0 s of static readings indicates fault
#define SENSOR_FLATLINE_EPSILON_VOLTS 0.0008f // Less than 0.8 mV variation = flatline

// =============================================================================
// BREATHING PATTERN ANALYSIS & PAUSE ALERT (PROTOTYPE) THRESHOLDS
// =============================================================================
// Breathing phase thresholds (in relative L/min units)
#define BREATH_START_THRESHOLD_FLOW   3.0f    // Flow required to confirm inhale/exhale
#define BREATH_END_HYSTERESIS_FLOW    1.2f    // Flow below which phase transitions to zero
#define LOW_FLOW_THRESHOLD_FLOW       2.0f    // Shallow breathing warning threshold
#define PAUSE_THRESHOLD_FLOW          1.0f    // Flow magnitude below which pause timer runs

// Breathing pause alert (prototype) configuration
// Duration without airflow before raising a pause alert event
#define PAUSE_ALERT_THRESHOLD_SEC     10.0f

// Physiological breathing period bounds (for rolling BPM calculation)
#define MIN_BREATH_PERIOD_MS          1200    // Equivalent to 50 BPM maximum
#define MAX_BREATH_PERIOD_MS          15000   // Equivalent to 4 BPM minimum
#define BPM_ROLLING_BUFFER_SIZE       8       // Breaths in rolling average window

// =============================================================================
// BATTERY MONITOR CONFIGURATION
// =============================================================================
#define BAT_DIVIDER_R_HIGH            100000.0f // 100 kOhm
#define BAT_DIVIDER_R_LOW             100000.0f // 100 kOhm
#define BAT_DIVIDER_RATIO             ((BAT_DIVIDER_R_LOW) / (BAT_DIVIDER_R_HIGH + BAT_DIVIDER_R_LOW)) // 0.50
#define BAT_VOLTAGE_FULL              4.20f
#define BAT_VOLTAGE_EMPTY             3.30f
#define BAT_LOW_PERCENT_THRESHOLD     20

// =============================================================================
// BLE SERVICE & CHARACTERISTIC UUIDS
// =============================================================================
#define BLE_DEVICE_NAME               "Airflow-Logger-ESP32"
#define BLE_SERVICE_UUID              "0000fff0-0000-1000-8000-00805f9b34fb"
#define BLE_CHAR_WAVEFORM_UUID        "0000fff1-0000-1000-8000-00805f9b34fb"
#define BLE_CHAR_BREATH_METRICS_UUID  "0000fff2-0000-1000-8000-00805f9b34fb"
#define BLE_CHAR_PAUSE_EVENT_UUID     "0000fff3-0000-1000-8000-00805f9b34fb"
#define BLE_CHAR_ENVIRONMENT_UUID     "0000fff4-0000-1000-8000-00805f9b34fb"
#define BLE_CHAR_BATTERY_UUID         "0000fff5-0000-1000-8000-00805f9b34fb"

// Status bitmasks
#define STATUS_BIT_NORMAL             (1 << 0)
#define STATUS_BIT_LOW_FLOW           (1 << 1)
#define STATUS_BIT_PAUSE_ALERT        (1 << 2)
#define STATUS_BIT_SENSOR_FAULT       (1 << 3)
#define STATUS_BIT_DEMO_MODE          (1 << 4)

#endif // CONFIG_H
