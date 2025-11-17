#ifndef BLE_LIVE_DATA_H
#define BLE_LIVE_DATA_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include "ble_uuids.h"

// Global status
extern bool bleLiveDataServiceActive;

// =========================
// MAIN FUNCTIONS
// =========================

/**
 * @brief Initializes the BLE live data service and sets up all required resources.
 * @return true if initialization was successful, otherwise false.
 */
bool initLiveDataService();

/**
 * @brief Starts the BLE live data service.
 * @return true if advertising started successfully, otherwise false.
 */
bool startLiveDataService();

/**
 * @brief Stops the BLE live data service and releases resources.
 * @return void
 */
void stopLiveDataService();

/**
 * @brief Sends a sensor measurement through the live data service to connected clients.
 * @param measurementJson Measurement data in JSON format.
 * @return true if the data was transferred successfully, otherwise false.
 */
bool sendLiveDataMeasurement(const String& measurementJson);

/**
 * @brief Updates the number of available sensor parameters in the service.
 * @param count Number of sensor parameters.
 * @return void
 */
void setLiveDataParameterCount(uint8_t count);

/**
 * @brief Checks whether the live data service is currently active.
 * @return true if the service is running, otherwise false.
 */
bool isLiveDataServiceActive();

/**
 * @brief Prints the current status of the live data service.
 * @return void
 */
void printLiveDataServiceStatus();

#endif
