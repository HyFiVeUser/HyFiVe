#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>

// =========================
// CONFIGURATION STRUCTURES
// =========================

#define MAX_PARAMETERS 10

// Sensor parameters from config.json
struct ConfigParameter {
    String id;           // "01", "02", etc.
    String uuid;         // UUID for backend
    String name;         // "temperature", "oxygen", etc.
    String unit;         // "°C", "mg/L", etc.
    String type;         // "temperature", "molar_doxy", etc.
    String icon;         // Icon for UI
};

// MainMCU configuration
struct MainMcuConfig {
    String datanodeAddress;
    String wifiSsid;
    String wifiPassword;
    String itemUuid;
    String itemToken;
    String bluetoothName;
    uint32_t poweroffThresholdMs;
    bool poweroffEnable;
    int32_t measurementStart;
    int32_t measurementStop;
};

// InterfaceMCU configuration
struct InterfaceMcuConfig {
    String sensorInterfaces[2];        // i2c, uart
    uint32_t i2cFrequency;
    uint32_t sensorBaudrate;
    float sensorVoltage;
    uint32_t sampleIntervalMs;
    uint32_t sensorPrepMs;
    String pythonFileNames[5];
    ConfigParameter parameters[MAX_PARAMETERS];
    uint8_t parameterCount;
};

// Complete system configuration
struct Config {
    MainMcuConfig mainMcu;
    InterfaceMcuConfig interfaceMcu;
};

// =========================
// CALIBRATION STRUCTURES
// =========================

// Calibration coefficients for a sensor
struct CalibrationCoeffs {
    String id;               // Sensor ID ("01", "02", etc.)
    String name;             // Sensor Name
    float coeffs[10];        // Up to 10 coefficients
    uint8_t coeffCount;      // Number of valid coefficients
};

// All calibration data
struct CalibrationData {
    CalibrationCoeffs parameters[MAX_PARAMETERS];
    uint8_t parameterCount;
};

// =========================
// GLOBAL CONFIGURATION
// =========================

extern Config systemConfig;
extern CalibrationData calibrationData;

// =========================
// FUNCTIONS
// =========================

/**
 * @brief Loads all configuration files from the SD card and populates the global structures.
 * @return true if all required files were read successfully, otherwise false.
 */
bool loadConfiguration();

/**
 * @brief Loads and parses the file config.json.
 * @return true if the file was read and fully parsed successfully, otherwise false.
 */
bool loadConfigJSON();

void sensorPrepDurationTime();

/**
 * @brief Loads and parses the file calib_coeff.json.
 * @return true if the calibration data was read successfully, otherwise false.
 */
bool loadCalibrationJSON();

/**
 * @brief Looks up calibration coefficients for a specific sensor ID.
 * @param id Sensor ID ("01", "02", etc.).
 * @return Pointer to the dataset or nullptr if no entry exists.
 */
CalibrationCoeffs* findCalibrationById(const String& id);

/**
 * @brief Prints the current system configuration over the serial interface.
 * @return void
 */
void printConfiguration();

/**
 * @brief Prints all loaded calibration data over the serial interface.
 * @return void
 */
void printCalibration();

#endif
