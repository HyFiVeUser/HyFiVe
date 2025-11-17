#ifndef BLE_UUIDS_H
#define BLE_UUIDS_H

// Centrally maintained UUIDs for all BLE services and characteristics
static constexpr char SERVICE_UUID[]                 = "12345678-1234-1234-1234-123456789abc";
static constexpr char LIVE_DATA_SERVICE_UUID[]       = "00000000-83a3-449e-afeb-52e55909e63c";
static constexpr char LIVE_DATA_PARAM_COUNT_UUID[]   = "00000100-83a3-449e-afeb-52e55909e63c";
static constexpr char LIVE_DATA_PARAM_READING_UUID[] = "00000200-83a3-449e-afeb-52e55909e63c";
static constexpr char CONFIG_SERVICE_UUID[]          = "00010000-83a3-449e-afeb-52e55909e63c";
static constexpr char CONFIG_CHARACTERISTIC_UUID[]   = "00010100-83a3-449e-afeb-52e55909e63c";
static constexpr char COLLECTED_DATA_SERVICE_UUID[]  = "00020000-83a3-449e-afeb-52e55909e63c";
static constexpr char SD_CARD_SERVICE_UUID[]         = "00020000-83a3-449e-afeb-52e55909e63c";
static constexpr char SD_CARD_CHARACTERISTIC_UUID[]  = "00020100-83a3-449e-afeb-52e55909e63c";
static constexpr char TIME_SERVICE_UUID[]            = "00030000-83a3-449e-afeb-52e55909e63c";
static constexpr char TIME_CHARACTERISTIC_UUID[]     = "00030100-83a3-449e-afeb-52e55909e63c";

#endif // BLE_UUIDS_H