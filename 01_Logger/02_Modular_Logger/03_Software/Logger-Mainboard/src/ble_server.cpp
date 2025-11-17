#include "ble_server.h"
#include "ble_uuids.h"
#include "logging.h"
#include "ble_config.h"

// =========================
// GLOBAL VARIABLES
// =========================
bool bleServerInitialized = false;
bool bleDeviceConnected = false;
NimBLEServer* bleServerInstance = nullptr;
static MyServerCallbacks* bleServerCallbacks = nullptr;
static bool advertisingStarted = false;

// =========================
// SERVER CALLBACKS
// =========================
void MyServerCallbacks::onConnect(NimBLEServer* server, NimBLEConnInfo& connectionInfo) {
   (void)server; (void)connectionInfo; // optional: silence unused warnings
    bleDeviceConnected = true;
    LOG_I("BLE device connected");
}

void MyServerCallbacks::onDisconnect(NimBLEServer* server, NimBLEConnInfo& connectionInfo, int reason) {
  (void)server; (void)connectionInfo; (void)reason;
    bleDeviceConnected = false;
    LOG_W("BLE device disconnected");
    resetConfigUploadState();
    delay(500);
    NimBLEDevice::startAdvertising();
    LOG_I("🔄 Advertising restarted");
}

// =========================
// MAIN FUNCTIONS
// =========================
bool initBLEServer(const char* deviceName) {
    if (bleServerInitialized) {
        LOG_W("BLE server already initialized");
        return true;
    }

    LOG_I(" Initializing BLE server: %s", deviceName);
    
    try {
        // Initialize BLE device
        NimBLEDevice::init(deviceName);
        NimBLEDevice::setMTU(517);         // ATT-MTU Ziel

        // Create BLE server
        bleServerInstance = NimBLEDevice::createServer();
        if (!bleServerInstance) {
            LOG_E("BLE server could not be created");
            return false;
        }

        // Set server callbacks
        bleServerCallbacks = new MyServerCallbacks();
        bleServerInstance->setCallbacks(bleServerCallbacks);

        bleServerInitialized = true;
        LOG_I("BLE server initialized successfully");
        return true;

    } catch (const std::exception& e) {
        LOG_E("BLE server initialization failed: %s", e.what());
        return false;
    }
}

void startBLEAdvertising() {
    if (!bleServerInitialized || advertisingStarted) {
        return;
    }
    
    try {
        NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
        
        // Add all service UUIDs
        pAdvertising->addServiceUUID(SERVICE_UUID);
        pAdvertising->addServiceUUID(LIVE_DATA_SERVICE_UUID);
        pAdvertising->addServiceUUID(CONFIG_SERVICE_UUID);
        pAdvertising->addServiceUUID(COLLECTED_DATA_SERVICE_UUID);
        pAdvertising->addServiceUUID(TIME_SERVICE_UUID);
        
        //! --- NEW: Place the name in the scan response ---
        String deviceName = getBLEDeviceName();

        NimBLEAdvertisementData scanResp;
        scanResp.setName(deviceName.c_str());
        pAdvertising->setScanResponseData(scanResp);

        NimBLEDevice::setDeviceName(deviceName.c_str());
        
        NimBLEDevice::startAdvertising();
        advertisingStarted = true;
        LOG_I("📡 BLE advertising started");

    } catch (const std::exception& e) {
        LOG_E("Error while starting advertising: %s", e.what());
    }
}

void stopBLEAdvertising() {
    if (advertisingStarted) {
        NimBLEDevice::stopAdvertising();
        advertisingStarted = false;
        LOG_W("BLE advertising stopped");
    }
}

bool isBLEDeviceConnected() {
    return bleDeviceConnected;
}

void shutdownBLEServer() {
    stopBLEAdvertising();
    
    if (bleServerCallbacks) {
        delete bleServerCallbacks;
        bleServerCallbacks = nullptr;
    }

    bleServerInstance = nullptr;
    bleServerInitialized = false;
    bleDeviceConnected = false;
    
    NimBLEDevice::deinit(false);
    LOG_W("BLE server shut down");
}

void printBLEServerStatus() {
    LOG_I("\n📡 === BLE SERVER STATUS ===");
    LOG_I("Server initialized: %s", bleServerInitialized ? "✅" : "❌");
    LOG_I("Device connected: %s", bleDeviceConnected ? "✅" : "❌");
    LOG_I("Advertising active: %s", advertisingStarted ? "✅" : "❌");
    LOG_I("===========================\n");
}
