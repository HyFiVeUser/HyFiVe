#ifndef BLE_TIME_H
#define BLE_TIME_H

#include <Arduino.h>

/**
 * @brief Initializes the BLE time service and sets up the characteristic for writing the time.
 * @return true on success, otherwise false.
 */
bool initTimeService();

/**
 * @brief Starts the BLE time service and makes it available to clients.
 * @return true if the service started successfully, otherwise false.
 */
bool startTimeService();

/**
 * @brief Prints the current status of the time service.
 */
void printTimeServiceStatus();

#endif