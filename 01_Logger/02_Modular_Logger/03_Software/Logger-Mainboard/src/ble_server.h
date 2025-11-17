#ifndef BLE_SERVER_H
#define BLE_SERVER_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>

// =========================
// BLE SERVER MANAGEMENT
// =========================

// Global BLE server status
extern bool bleServerInitialized;
extern bool bleDeviceConnected;
extern NimBLEServer* bleServerInstance;

// Server Callbacks - RENAMED to avoid conflict
class MyServerCallbacks : public NimBLEServerCallbacks {
public:
    void onConnect(NimBLEServer* server, NimBLEConnInfo& connectionInfo) override;
  void onDisconnect(NimBLEServer* server, NimBLEConnInfo& connectionInfo, int reason) override;
};

// =========================
// MAIN FUNCTIONS
// =========================

/**
 * @brief Initializes the central BLE server.
 * @param deviceName Name to use for BLE advertising.
 * @return true if the server was created successfully, otherwise false.
 */
bool initBLEServer(const char* deviceName);

/**
 * @brief Starts BLE advertising with all available service UUIDs.
 * @return void
 */
void startBLEAdvertising();

/**
 * @brief Stops BLE advertising.
 * @return void
 */
void stopBLEAdvertising();

/**
 * @brief Checks whether a device is currently connected to the server.
 * @return true if a device is connected, otherwise false.
 */
bool isBLEDeviceConnected();

/**
 * @brief Shuts down the BLE server and releases resources.
 * @return void
 */
void shutdownBLEServer();

/**
 * @brief Prints the current status of the BLE server.
 * @return void
 */
void printBLEServerStatus();

#endif
