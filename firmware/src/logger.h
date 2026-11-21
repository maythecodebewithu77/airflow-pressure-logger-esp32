#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include "config.h"
#include "airflow.h"
#include "breathing.h"
#include "bme280_reader.h"
#include "battery.h"

class SerialTelemetryLogger {
public:
    SerialTelemetryLogger();

    void begin(unsigned long baud = SERIAL_BAUD_RATE);

    // Stream waveform sample for Serial Plotter
    void logPlotterSample(const AirflowSample &sample, const BreathingMetrics &metrics);

    // Stream full CSV row to UART
    void logCsvRow(const AirflowSample &sample,
                   const BreathingMetrics &metrics,
                   const Bme280Data &env,
                   const BatteryState &bat);

    // Output human-readable events
    void logEvent(const char *tag, const char *message);
    void logPauseAlert(uint32_t duration_ms);

private:
    bool m_header_printed;
    void printCsvHeader();
};

#endif // LOGGER_H
