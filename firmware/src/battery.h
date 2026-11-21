#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>
#include "config.h"

struct BatteryState {
    uint32_t timestamp_ms;
    float voltage_volts;       // Cell terminal voltage (V)
    uint8_t percentage;        // Estimated State of Charge (0 - 100%)
    bool is_low_battery;       // True if percentage < BAT_LOW_PERCENT_THRESHOLD
    bool is_charging;          // True if USB charging detected
};

class BatteryMonitor {
public:
    BatteryMonitor();

    void begin();

    // Sample ADC and estimate state of charge (called at ~0.5 Hz)
    BatteryState update();

    void setDemoMode(bool enabled) { m_demo_mode = enabled; }
    const BatteryState& getState() const { return m_state; }

private:
    BatteryState m_state;
    bool m_demo_mode;
    float m_filtered_voltage;

    uint8_t estimateSoc(float volts);
};

#endif // BATTERY_H
