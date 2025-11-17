#ifndef BLE_INTEGRATION_H
#define BLE_INTEGRATION_H

#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "init.h"

// =========================
// BLE INTEGRATION INTERFACE
// =========================

/**
 * @brief Initializes the entire BLE system including services and advertising configuration.
 * @return true if all components were initialized successfully, otherwise false.
 */
bool initializeBLE();

bool transferActive();

/**
 * @brief Starts the BLE connection to the app.
 * @return true if advertising was started and connections are possible, otherwise false.
 */
bool startBLE();

/**
 * @brief Sends all current sensor data over BLE to connected clients.
 * @return void
 */
void transmitSensorData();

#endif
