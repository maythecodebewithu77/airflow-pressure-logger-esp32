#include "logger.h"

SerialTelemetryLogger::SerialTelemetryLogger()
    : m_header_printed(false) {}

void SerialTelemetryLogger::begin(unsigned long baud) {
    Serial.begin(baud);
    delay(200);
    Serial.println();
    Serial.println(F("=================================================================="));
    Serial.println(F("Breathing Airflow and Mask-Pressure Logger (learning prototype)"));
    Serial.println(F("Author: Vigneshwar S (SRM Institute of Science and Technology)"));
    Serial.println(F("Safety Notice: Educational use only. NOT A MEDICAL DEVICE."));
    Serial.println(F("=================================================================="));
    printCsvHeader();
}

void SerialTelemetryLogger::printCsvHeader() {
    if (SERIAL_CSV_MODE && !m_header_printed) {
        Serial.println(F("timestamp_ms,raw_volts,pressure_pa,filt_pressure_pa,flow_rel_lmin,bpm,peak_insp,peak_exp,ie_ratio,status_flags,pause_duration_s,temp_c,humidity_pct,baro_pa,bat_v,bat_pct"));
        m_header_printed = true;
    }
}

void SerialTelemetryLogger::logPlotterSample(const AirflowSample &sample, const BreathingMetrics &metrics) {
    if (!SERIAL_PLOTTER_MODE) return;
    // Format compatible with Arduino Serial Plotter
    Serial.print(F("Raw_dP:"));
    Serial.print(sample.raw_pressure_pa, 1);
    Serial.print(F(",Filt_dP:"));
    Serial.print(sample.filtered_pressure_pa, 1);
    Serial.print(F(",Flow:"));
    Serial.print(sample.relative_flow_lmin, 1);
    Serial.print(F(",BPM:"));
    Serial.println(metrics.bpm, 1);
}

void SerialTelemetryLogger::logCsvRow(const AirflowSample &sample,
                                     const BreathingMetrics &metrics,
                                     const Bme280Data &env,
                                     const BatteryState &bat) {
    if (!SERIAL_CSV_MODE) return;

    Serial.print(sample.timestamp_ms);
    Serial.print(',');
    Serial.print(sample.raw_voltage, 4);
    Serial.print(',');
    Serial.print(sample.raw_pressure_pa, 2);
    Serial.print(',');
    Serial.print(sample.filtered_pressure_pa, 2);
    Serial.print(',');
    Serial.print(sample.relative_flow_lmin, 2);
    Serial.print(',');
    Serial.print(metrics.bpm, 1);
    Serial.print(',');
    Serial.print(metrics.peak_insp_flow_lmin, 2);
    Serial.print(',');
    Serial.print(metrics.peak_exp_flow_lmin, 2);
    Serial.print(',');
    Serial.print(metrics.ie_ratio, 2);
    Serial.print(',');
    Serial.print(metrics.status_flags);
    Serial.print(',');
    Serial.print(metrics.current_pause_duration_s, 1);
    Serial.print(',');
    Serial.print(env.temperature_c, 2);
    Serial.print(',');
    Serial.print(env.humidity_percent, 1);
    Serial.print(',');
    Serial.print(env.pressure_pa, 0);
    Serial.print(',');
    Serial.print(bat.voltage_volts, 2);
    Serial.print(',');
    Serial.println(bat.percentage);
}

void SerialTelemetryLogger::logEvent(const char *tag, const char *message) {
    Serial.print(F("["));
    Serial.print(tag);
    Serial.print(F("] "));
    Serial.println(message);
}

void SerialTelemetryLogger::logPauseAlert(uint32_t duration_ms) {
    Serial.print(F("[ALERT] Breathing pause alert (prototype): "));
    Serial.print((float)duration_ms / 1000.0f, 1);
    Serial.println(F(" seconds"));
}
