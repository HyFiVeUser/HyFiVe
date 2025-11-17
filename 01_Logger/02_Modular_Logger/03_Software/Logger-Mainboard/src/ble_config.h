#ifndef BLE_CONFIG_H
#define BLE_CONFIG_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include <esp_mac.h>
#include "ble_uuids.h"
#include "configuration_receiver.h"
#include "NVSPreferences.h"

// =========================
// CONFIG SERVICE
// =========================

// Global status
extern bool bleConfigUploadActive;
extern String requestType;
inline extern RTC_DATA_ATTR uint8_t errorSDCount = 0;

// Transfer packet structure (local copy for the config service)
struct ConfigTransferPacket {
    struct {
        uint16_t total_num_packets;
        uint16_t whole_packet_checksum;
    } __attribute__((packed)) metadata;
    
    struct {
        uint16_t index;
        uint16_t checksum;
        uint16_t payload_size;
        uint8_t payload[502];
    } __attribute__((packed)) chunk;
} __attribute__((packed));

// =========================
// MAIN FUNCTIONS
// =========================

/**
 * @brief Initializes the BLE configuration service and sets up all required resources.
 * @return true if the service was initialized correctly, otherwise false.
 */
bool initConfigService();

/**
 * @brief Starts the BLE configuration service and makes it available for clients.
 * @return true if advertising started successfully, otherwise false.
 */
bool startConfigService();

/**
 * @brief Stops the BLE configuration service and releases resources.
 * @return void
 */
void stopConfigService();

/**
 * @brief Checks whether a configuration upload is currently active.
 * @return true if an upload is running, otherwise false.
 */
bool isConfigUploadActive();

/**
 * @brief Prints the current status of the configuration service.
 * @return void
 */
void printConfigServiceStatus();

/**
 * @brief Resets a running configuration upload.
 * @param reason Optional reason for the reset (e.g., timeout or disconnect).
 * @return void
 */
void resetConfigUploadState();

/**
 * @brief Periodically checks for a timeout or connection loss during a configuration upload.
 */
void pollConfigUploadTimeout();

/**
 * @brief Returns the BLE device name built from the prefix and stored ID.
 * @return Fully composed device name for advertising.
 */
inline String getBLEDeviceName() {
    String storedName = readPrefs(true);

    if (storedName.startsWith("osbk-")) {
        return storedName;
    }

    uint8_t mac[6] = {0};
    esp_err_t result = esp_read_mac(mac, ESP_MAC_BT);
    if (result != ESP_OK) {
        return String("osbk-HER-") + storedName;
    }

    char macString[13];
    for (size_t i = 0; i < 6; ++i) {
        sprintf(&macString[i * 2], "%02x", mac[i]);
    }
    macString[12] = '\0';

    return String("osbk-HER-") + String(macString);
}

#endif
