#include "ble_task.h"
#include "ble_server.h"
#include "ble_live_data.h"
#include "ble_config.h"
#include "ble_sd_card.h"
#include "init.h"
#include "logging.h"

// =========================
// GLOBAL TASK OBJECTS
// =========================
TaskHandle_t bleTaskHandle = nullptr;
EventGroupHandle_t bleEventGroup = nullptr;
SemaphoreHandle_t configUploadMutex = nullptr;

// Task Status
static bool taskRunning = false;

// =========================
// TASK INITIALIZATION
// =========================
bool initializeBLETask() {
    LOG_I(" Initializing BLE task system...");
    
    // Create event group
    bleEventGroup = xEventGroupCreate();
    if (!bleEventGroup) {
        LOG_E("Event group creation failed");
        return false;
    }
    
    // Create mutex for configuration upload
    configUploadMutex = xSemaphoreCreateMutex();
    if (!configUploadMutex) {
        LOG_E("Config upload mutex creation failed");
        return false;
    }
    
    // Create BLE task (on core 1)
    BaseType_t result = xTaskCreatePinnedToCore(
        bleTaskFunction,           // Task function
        "BLE_Task",               // Task name
        BLE_TASK_STACK_SIZE,      // Stack size
        nullptr,                  // Parameter
        BLE_TASK_PRIORITY,        // Priority
        &bleTaskHandle,           // Task handle
        BLE_TASK_CORE             //! Core 0
    );
    
    if (result != pdPASS) {
        LOG_E("BLE task creation failed");
        return false;
    }

    LOG_I("BLE task system initialized successfully");
    return true;
}

// =========================
// BLE TASK MAIN FUNCTION
// =========================
void bleTaskFunction(void* parameter) {
    LOG_I("BLE task started on core 1");
    
    taskRunning = true;

    while (taskRunning) {
        // BLE connection management (when connected)
        if (isBLEDeviceConnected()) {
            // Heartbeat and status updates
            static unsigned long lastHeartbeat = 0;
            unsigned long currentTime = millis();
            
            if (currentTime - lastHeartbeat > 5000) { // 5 seconds
                // Only when no config upload is active - use functions from service modules
                if (!isConfigUploadActive() && !isSDCardTransferActive()) {
                    // Heartbeat could be sent here
                }
                lastHeartbeat = currentTime;
            }
        }
        
        // Short pause for other tasks
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    LOG_W("BLE task stopped");
    vTaskDelete(nullptr);
}

// =========================
// HELPER FUNCTIONS
// =========================
void handleStartConnection() {
    LOG_I("📡 Starting BLE connection...");
    
    // Server is already initialized through ble_integration
    // Only set events here
    
    LOG_I("BLE connection ready");
    LOG_I("📱 Waiting for app connection...");
    
    // Set event for other tasks
    xEventGroupSetBits(bleEventGroup, BLE_EVENT_CONFIG_UPLOAD_STOP);
}

void handleSendSensorData(const char* data, size_t dataSize) {
    if (!isBLEDeviceConnected()) {
        return; // No connection - ignore
    }
    
    // Check config upload status - use functions from service modules
    if (isConfigUploadActive() || isSDCardTransferActive()) {
        LOG_W("📡 Sensor data paused - transfer in progress");
        return;
    }
    
    // Send sensor data
    String sensorJson = String(data);
    if (sendLiveDataMeasurement(sensorJson)) {
        // Sent successfully
    }
}

void handleConfigUploadActive() {
    LOG_W("🔒 Config upload started - sensor data paused");

    // Event for other tasks
    xEventGroupSetBits(bleEventGroup, BLE_EVENT_CONFIG_UPLOAD_START);
}

void handleConfigUploadInactive() {
    LOG_I("🔓 Config upload finished - sensor data resumed");
    
    // Event for other tasks
    xEventGroupSetBits(bleEventGroup, BLE_EVENT_CONFIG_UPLOAD_STOP);
}

// =========================
// PUBLIC FUNCTIONS
// =========================
bool startBLEConnection() {
    handleStartConnection();
    return true;
}

// REMOVED - use function from ble_config.cpp instead
// bool isConfigUploadActive() { ... }

// REMOVED - use function from ble_sd_card.cpp instead
// bool isSDCardTransferActive() { ... }