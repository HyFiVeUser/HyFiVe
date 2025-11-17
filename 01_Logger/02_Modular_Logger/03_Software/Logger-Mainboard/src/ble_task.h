#ifndef BLE_TASK_H
#define BLE_TASK_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/event_groups.h>

// BLE modules from the BLE software

//#include "ble_traffic_monitor.h"

// =========================
// TASK CONFIGURATION
// =========================
#define BLE_TASK_STACK_SIZE 12288
#define BLE_TASK_PRIORITY 1
#define BLE_TASK_CORE 0 //!

// Event bits for task synchronization
#define BLE_EVENT_CONFIG_UPLOAD_START   BIT0
#define BLE_EVENT_CONFIG_UPLOAD_STOP    BIT1
#define BLE_EVENT_SENSOR_DATA_READY     BIT2
#define BLE_EVENT_SHUTDOWN              BIT3

// =========================
// GLOBAL TASK OBJECTS
// =========================
extern TaskHandle_t bleTaskHandle;
extern EventGroupHandle_t bleEventGroup;
extern SemaphoreHandle_t configUploadMutex;

// =========================
// TASK FUNCTIONS
// =========================

/**
 * @brief Initializes the BLE task system and sets up synchronization objects.
 * @return true if all resources were created successfully, otherwise false.
 */
bool initializeBLETask();

/**
 * @brief Main function of the BLE task (runs on core 1).
 * @param parameter Optional task parameters (not used).
 * @return void
 */
void bleTaskFunction(void* parameter);

/**
 * @brief Internal helper function to start a BLE connection.
 * @return void
 */
void handleStartConnection();

/**
 * @brief Internal helper function to transmit sensor data.
 * @param data Pointer to the data buffer to send.
 * @param dataSize Size of the data buffer in bytes.
 * @return void
 */
void handleSendSensorData(const char* data, size_t dataSize);

/**
 * @brief Marks the configuration upload as active.
 * @return void
 */
void handleConfigUploadActive();

/**
 * @brief Marks the configuration upload as inactive.
 * @return void
 */
void handleConfigUploadInactive();

/**
 * @brief Starts the BLE connection.
 * @return true if the connection was started, otherwise false.
 */
bool startBLEConnection();

/**
 * @brief Checks whether a configuration upload is currently running.
 * @return true if an upload is active, otherwise false.
 */
bool isConfigUploadActive();

/**
 * @brief Checks whether an SD card transfer is active.
 * @return true if a transfer is running, otherwise false.
 */
bool isSDCardTransferActive();

#endif
