#ifndef BLE_SD_CARD_H
#define BLE_SD_CARD_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <NimBLEUtils.h>
#include "ble_uuids.h"

// BT‑SD Protocol Commands (unchanged, OSBK semantics)
#define CMD_MGC 0x00  // Measurements Get Count
#define CMD_MNC 0x01  // Measurements New Count
#define CMD_MGR 0x02  // Measurements Get Range
#define CMD_MNR 0x03  // Measurements New Range

// Global busy flag (used by OSBK to pause live streaming while SD transfer runs)
extern bool bleSDCardTransferActive;

// =========================
// Unified Transfer Packet (matches SDCardService)
// =========================
struct Transfer_Packet_t {
  struct {
    uint16_t total_num_packets = 1;
    uint16_t whole_packet_checksum = 0xDEAD;
  } __attribute__((packed)) metadata;

  struct {
    uint16_t index = 0;
    uint16_t checksum = 0xDEAD;
    uint16_t payload_size = 0;
    uint8_t  payload[502] = {};
  } __attribute__((packed)) chunk;
} __attribute__((packed));

// Public API (kept as in OSBK)
bool initSDCardService();
bool startSDCardService();
void stopSDCardService();
bool isSDCardTransferActive();
void printBLESDCardServiceStatus();

#endif // BLE_SD_CARD_H