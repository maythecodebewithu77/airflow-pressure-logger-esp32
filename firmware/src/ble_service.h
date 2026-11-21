#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "config.h"
#include "airflow.h"
#include "breathing.h"
#include "bme280_reader.h"
#include "battery.h"

#pragma pack(push, 1)
struct BleWaveformPacket {
    uint32_t timestamp_ms;
    int16_t pressure_pa_x10;
    int16_t flow_rel_x10;
};

struct BleMetricsPacket {
    uint16_t bpm_x10;
    uint16_t peak_insp_flow_x10;
    uint16_t peak_exp_flow_x10;
    uint16_t ie_ratio_x10;
    uint8_t status_flags;
};

struct BlePausePacket {
    uint32_t event_start_ms;
    uint32_t duration_ms;
};

struct BleEnvPacket {
    int16_t temperature_c_x100;
    uint16_t humidity_rh_x100;
    uint32_t pressure_pa;
};

struct BleBatteryPacket {
    uint16_t millivolts;
    uint8_t percent;
    uint8_t charging_flag;
};
#pragma pack(pop)

class BleLoggerService : public BLEServerCallbacks {
public:
    BleLoggerService();

    void begin(const char *deviceName = BLE_DEVICE_NAME);

    // Stream live waveform sample to connected client
    void notifyWaveform(const AirflowSample &sample);

    // Stream breathing metrics summary
    void notifyMetrics(const BreathingMetrics &metrics);

    // Notify breathing pause alert event
    void notifyPauseEvent(uint32_t start_ms, uint32_t duration_ms);

    // Stream environmental conditions
    void notifyEnvironment(const Bme280Data &env);

    // Stream battery status
    void notifyBattery(const BatteryState &bat);

    bool isConnected() const { return m_device_connected; }

    // BLEServerCallbacks overrides
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;

private:
    BLEServer *m_server;
    BLECharacteristic *m_char_waveform;
    BLECharacteristic *m_char_metrics;
    BLECharacteristic *m_char_pause;
    BLECharacteristic *m_char_env;
    BLECharacteristic *m_char_battery;

    bool m_device_connected;
    bool m_old_device_connected;
};

#endif // BLE_SERVICE_H
