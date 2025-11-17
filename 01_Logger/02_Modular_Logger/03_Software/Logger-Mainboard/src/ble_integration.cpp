#include "ble_integration.h"
#include "ble_task.h"
#include "ble_server.h"
#include "ble_live_data.h"
#include "ble_config.h"
#include "ble_sd_card.h"
#include "ble_time.h"
#include "logging.h"

// =========================
// PRIVATE STATE
// =========================
static bool bleInitialized = false;
static bool bleActive = false;

// =========================
// HELPER FUNCTIONS - DECLARED BEFORE transmitSensorData!
// =========================
String createISO8601Timestamp() {
    DateTime now = rtc.now();
    char timestamp[20];
    sprintf(timestamp, "%04d-%02d-%02dT%02d:%02d:%02d",
            now.year(), now.month(), now.day(),
            now.hour(), now.minute(), now.second());
    return String(timestamp);
}

String createAWISensorJSON(const ConfigSensorInfo& sensor) {
    String json = "{";
    json += "\"id\":\"" + sensor.id + "\",";
    json += "\"nvalue\":" + String(sensor.currentValue, 4) + ",";
    json += "\"timestamp\":\"" + createISO8601Timestamp() + "\",";
    json += "\"paramUuid\":\"" + sensor.uuid + "\",";
    json += "\"lat\":0,";
    json += "\"lng\":0,";
    json += "\"svalue\":\"\"";
    json += "}";
    return json;
}

// =========================
// MAIN FUNCTIONS
// =========================
bool initializeBLE() {
    if (bleInitialized) {
        LOG_W("BLE already initialized");
        return true;
    }

    LOG_I(" Initializing BLE integration...");
    
    // 1. Initialize BLE task
    if (!initializeBLETask()) {
        LOG_E("BLE task initialization failed");
        return false;
    }
    
    // 2. Initialize BLE server
    String bleDeviceName = getBLEDeviceName();
    if (!initBLEServer(bleDeviceName.c_str())) {
        LOG_E("BLE server initialization failed");
        return false;
    }
    
    // 3. Initialize all services (but do not start them yet)
    initLiveDataService();
    initConfigService();
    initSDCardService();
    initTimeService();
    
    bleInitialized = true;
    LOG_I("BLE integration initialized successfully");
    return true;
}

bool startBLE() {
    if (!bleInitialized) {
        LOG_E("BLE not initialized");
        return false;
    }

    if (bleActive) {
        LOG_I("📡 BLE already active");
        return true;
    }

    LOG_I("📡 Starting BLE system...");
    
    // Start services
    startLiveDataService();
    startConfigService();
    startSDCardService();
    startTimeService();
    
    // Start advertising
    startBLEAdvertising();
    
    // Activate BLE task
    if (startBLEConnection()) {
        bleActive = true;
        
        // Set sensor count
        setLiveDataParameterCount(activeSensorCount);
        
        LOG_I("BLE started successfully");
        return true;
    } else {
        LOG_E("BLE start failed");
        return false;
    }
}

bool transferActive()
{
    return bleConfigUploadActive || bleSDCardTransferActive;
}

void transmitSensorData() {
    if (!bleActive || !isBLEDeviceConnected()) {
        return;
    }

    // Check global flags
    if (bleConfigUploadActive || bleSDCardTransferActive) {
        return; // Blocked during transfers
    }
    
    // Send sensor data
    for (uint8_t i = 0; i < activeSensorCount; i++) {
        if (!activeSensors[i].active || !activeSensors[i].valueValid) {
            continue;
        }
        
        String sensorJson = createAWISensorJSON(activeSensors[i]);
        sendLiveDataMeasurement(sensorJson);
    }
}
