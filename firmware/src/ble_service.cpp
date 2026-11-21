#include "ble_service.h"

BleLoggerService::BleLoggerService()
    : m_server(nullptr),
      m_char_waveform(nullptr),
      m_char_metrics(nullptr),
      m_char_pause(nullptr),
      m_char_env(nullptr),
      m_char_battery(nullptr),
      m_device_connected(false),
      m_old_device_connected(false) {}

void BleLoggerService::begin(const char *deviceName) {
    BLEDevice::init(deviceName);

    // Create BLE Server
    m_server = BLEDevice::createServer();
    m_server->setCallbacks(this);

    // Create Primary GATT Service
    BLEService *service = m_server->createService(BLE_SERVICE_UUID);

    // 1. Waveform Characteristic (Notify)
    m_char_waveform = service->createCharacteristic(
        BLE_CHAR_WAVEFORM_UUID,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    m_char_waveform->addDescriptor(new BLE2902());

    // 2. Breathing Metrics Summary Characteristic (Notify | Read)
    m_char_metrics = service->createCharacteristic(
        BLE_CHAR_BREATH_METRICS_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
    );
    m_char_metrics->addDescriptor(new BLE2902());

    // 3. Pause Event Characteristic (Notify | Read)
    m_char_pause = service->createCharacteristic(
        BLE_CHAR_PAUSE_EVENT_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
    );
    m_char_pause->addDescriptor(new BLE2902());

    // 4. Environmental Characteristic (Notify | Read)
    m_char_env = service->createCharacteristic(
        BLE_CHAR_ENVIRONMENT_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
    );
    m_char_env->addDescriptor(new BLE2902());

    // 5. Battery Characteristic (Notify | Read)
    m_char_battery = service->createCharacteristic(
        BLE_CHAR_BATTERY_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ
    );
    m_char_battery->addDescriptor(new BLE2902());

    // Start service
    service->start();

    // Configure and start advertising
    BLEAdvertising *advertising = BLEDevice::getAdvertising();
    advertising->addServiceUUID(BLE_SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->setMinPreferred(0x06); // Functions that help with iPhone connections issue
    advertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
}

void BleLoggerService::onConnect(BLEServer* pServer) {
    m_device_connected = true;
}

void BleLoggerService::onDisconnect(BLEServer* pServer) {
    m_device_connected = false;
    // Restart advertising to allow auto-reconnection
    BLEDevice::startAdvertising();
}

void BleLoggerService::notifyWaveform(const AirflowSample &sample) {
    if (!m_device_connected || !m_char_waveform) return;

    BleWaveformPacket packet;
    packet.timestamp_ms = sample.timestamp_ms;
    packet.pressure_pa_x10 = (int16_t)constrain((int32_t)(sample.clean_pressure_pa * 10.0f), -32768, 32767);
    packet.flow_rel_x10 = (int16_t)constrain((int32_t)(sample.relative_flow_lmin * 10.0f), -32768, 32767);

    m_char_waveform->setValue((uint8_t*)&packet, sizeof(packet));
    m_char_waveform->notify();
}

void BleLoggerService::notifyMetrics(const BreathingMetrics &metrics) {
    if (!m_device_connected || !m_char_metrics) return;

    BleMetricsPacket packet;
    packet.bpm_x10 = (uint16_t)constrain((int32_t)(metrics.bpm * 10.0f), 0, 65535);
    packet.peak_insp_flow_x10 = (uint16_t)constrain((int32_t)(metrics.peak_insp_flow_lmin * 10.0f), 0, 65535);
    packet.peak_exp_flow_x10 = (uint16_t)constrain((int32_t)(metrics.peak_exp_flow_lmin * 10.0f), 0, 65535);
    packet.ie_ratio_x10 = (uint16_t)constrain((int32_t)(metrics.ie_ratio * 10.0f), 0, 65535);
    packet.status_flags = metrics.status_flags;

    m_char_metrics->setValue((uint8_t*)&packet, sizeof(packet));
    m_char_metrics->notify();
}

void BleLoggerService::notifyPauseEvent(uint32_t start_ms, uint32_t duration_ms) {
    if (!m_device_connected || !m_char_pause) return;

    BlePausePacket packet;
    packet.event_start_ms = start_ms;
    packet.duration_ms = duration_ms;

    m_char_pause->setValue((uint8_t*)&packet, sizeof(packet));
    m_char_pause->notify();
}

void BleLoggerService::notifyEnvironment(const Bme280Data &env) {
    if (!m_device_connected || !m_char_env) return;

    BleEnvPacket packet;
    packet.temperature_c_x100 = (int16_t)(env.temperature_c * 100.0f);
    packet.humidity_rh_x100 = (uint16_t)(env.humidity_percent * 100.0f);
    packet.pressure_pa = (uint32_t)env.pressure_pa;

    m_char_env->setValue((uint8_t*)&packet, sizeof(packet));
    m_char_env->notify();
}

void BleLoggerService::notifyBattery(const BatteryState &bat) {
    if (!m_device_connected || !m_char_battery) return;

    BleBatteryPacket packet;
    packet.millivolts = (uint16_t)(bat.voltage_volts * 1000.0f);
    packet.percent = bat.percentage;
    packet.charging_flag = bat.is_charging ? 1 : 0;

    m_char_battery->setValue((uint8_t*)&packet, sizeof(packet));
    m_char_battery->notify();
}
