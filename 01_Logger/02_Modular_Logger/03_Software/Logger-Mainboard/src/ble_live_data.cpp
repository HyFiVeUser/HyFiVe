#include "ble_live_data.h"
#include "ble_server.h"
#include "logging.h"

// =========================
// GLOBAL VARIABLES
// =========================
bool bleLiveDataServiceActive = false;
static bool serviceInitialized = false;
static NimBLEService* liveDataService = nullptr;
static NimBLECharacteristic* liveDataParameterCountCharacteristic = nullptr;
static NimBLECharacteristic* liveDataMeasurementCharacteristic = nullptr;
static uint8_t configuredParameterCount = 0;

// =========================
// MAIN FUNCTIONS
// =========================
bool initLiveDataService() {
    if (serviceInitialized) {
        LOG_W("Live Data Service already initialized");
        return true;
    }
    
    if (!bleServerInitialized || !bleServerInstance) {
        LOG_E("BLE server not available");
        return false;
    }
    
    LOG_I(" Initializing Live Data Service...");
    
    try {
        // Create service
        liveDataService = bleServerInstance->createService(LIVE_DATA_SERVICE_UUID);
        if (!liveDataService) {
            LOG_E("Live Data Service could not be created");
            return false;
        }

        // Count characteristic

        liveDataParameterCountCharacteristic = liveDataService->createCharacteristic(
            LIVE_DATA_PARAM_COUNT_UUID,
            NIMBLE_PROPERTY::READ
        );

        // Reading characteristic
        liveDataMeasurementCharacteristic = liveDataService->createCharacteristic(
            LIVE_DATA_PARAM_READING_UUID,
            NIMBLE_PROPERTY::READ |
            NIMBLE_PROPERTY::NOTIFY
        );

        if (!liveDataParameterCountCharacteristic || !liveDataMeasurementCharacteristic) {
            LOG_E("Characteristics could not be created");
            return false;
        }

        // Descriptor for notifications
        liveDataMeasurementCharacteristic->createDescriptor("2902");

        // Set default values
        liveDataParameterCountCharacteristic->setValue("0");
        liveDataMeasurementCharacteristic->setValue("No data");

        serviceInitialized = true;
        LOG_I("Live Data Service initialized");
        return true;
        
    } catch (const std::exception& e) {
        LOG_E("Live Data Service error: %s", e.what());
        return false;
    }
}

bool startLiveDataService() {
    if (!serviceInitialized) {
        LOG_E("Live Data Service not initialized");
        return false;
    }
    
    if (bleLiveDataServiceActive) {
        LOG_W("Live Data Service already active");
        return true;
    }
    
    try {
        liveDataService->start();
        bleLiveDataServiceActive = true;
        LOG_I("Live Data Service started");
        return true;
        
    } catch (const std::exception& e) {
        LOG_E("Live Data Service start failed: %s", e.what());
        return false;
    }
}

void stopLiveDataService() {
    if (!bleLiveDataServiceActive) {
        return;
    }
    
    // Service cannot truly be stopped in ESP32 BLE
    // We simply mark it as inactive
    bleLiveDataServiceActive = false;
    LOG_W("Live Data Service stopped");
}

bool sendLiveDataMeasurement(const String& measurementJson) {
    if (!bleLiveDataServiceActive || !bleDeviceConnected) {
        return false;
    }
    
    if (!liveDataMeasurementCharacteristic) {
        return false;
    }

    try {
        liveDataMeasurementCharacteristic->setValue(measurementJson.c_str());
        liveDataMeasurementCharacteristic->notify();
        return true;

    } catch (const std::exception& e) {
        LOG_E("Live Data send error: %s", e.what());
        return false;
    }
}

void setLiveDataParameterCount(uint8_t count) {
    configuredParameterCount = count;

    if (liveDataParameterCountCharacteristic) {
        String countStr = String(count);
        liveDataParameterCountCharacteristic->setValue(countStr.c_str());
        LOG_I("Live Data: %d parameters configured", count);
    }
}

bool isLiveDataServiceActive() {
    return bleLiveDataServiceActive;
}

void printLiveDataServiceStatus() {
    LOG_I("\n=== LIVE DATA SERVICE STATUS ===");
    LOG_I("Service initialized: %s", serviceInitialized ? "✅" : "❌");
    LOG_I("Service active: %s", bleLiveDataServiceActive ? "✅" : "❌");
    LOG_I("Parameter Count: %d", configuredParameterCount);
    LOG_I("==================================\n");
}