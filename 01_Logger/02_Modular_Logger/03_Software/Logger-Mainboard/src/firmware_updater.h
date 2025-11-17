#pragma once

#include <Arduino.h>

/**
 * @brief Checks the online CRC32.txt to see whether a new firmware is available
 *        and performs the OTA update if required.
 *
 * On a successful update the function requests a restart of the ESP32 and
 * normally only returns when either no update is required or an error occurs.
 *
 * @return true if the firmware is already up to date or an update was completed
 *         successfully. false if an error occurred during the check/update.
 */
bool performFirmwareUpdateIfAvailable();

