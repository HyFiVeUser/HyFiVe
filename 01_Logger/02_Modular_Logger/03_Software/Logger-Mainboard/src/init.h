#ifndef SYSTEM_INIT_H
#define SYSTEM_INIT_H

#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include <SD.h>
#include <WiFi.h>
#include <time.h>
#include <ArduinoJson.h>

// =========================
// HARDWARE PIN DEFINITIONS
// =========================

// SD card
#define SD_CS_PIN 18
#define SD_MOSI_PIN 1
#define SD_MISO_PIN 38
#define SD_SCK_PIN 2

// I2C sensors
#define SENSOR_SDA_PIN 4
#define SENSOR_SCL_PIN 5
#define SENSOR_I2C_FREQUENCY 100000

// I2C BMS + RTC (shared bus)
#define BMS_SDA_PIN 21
#define BMS_SCL_PIN 14
#define BMS_I2C_FREQUENCY 100000

// RTC alarm pin for wakeup
#define RTC_ALARM_PIN GPIO_NUM_17 // ESP32-S3 RTC‑Pins GPIO0 … GPIO21 

// =========================
// SYSTEM CONFIGURATION
// =========================

// File names on SD card
#define CONFIG_UPDATE_DIR "/config_update"
#define CONFIG_ACTIVE_DIR "/config"
#define CONFIG_UPDATE_FILE CONFIG_UPDATE_DIR "/config.json"
#define CONFIG_FILE CONFIG_ACTIVE_DIR "/config.json"
#define CALIB_FILE CONFIG_UPDATE_DIR "/calib_coeff.json"

// WiFi & NTP Settings
#define WIFI_TIMEOUT_MS 15000
#define NTP_TIMEOUT_MS 10000
#define GMT_OFFSET_SEC 0      // UTC - no time zone offset
#define DAYLIGHT_OFFSET_SEC 0  // UTC - no daylight saving time

// NTP Server
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.nist.gov" 
#define NTP_SERVER_3 "de.pool.ntp.org"

// BMS
#define BQ40Z80_ADDR 0x0B
#define VOLTAGE_REG 0x09
#define Register_RSOC 0xD

// =========================
// GLOBAL SYSTEM OBJECTS
// =========================

extern RTC_DS3231 rtc;              // RTC instance
extern TwoWire I2C_SENSORS;         // Sensor I2C bus
extern uint64_t measurementCycle;   // Measurement cycle in seconds
// =========================
// MAIN FUNCTIONS
// =========================

/**
 * @brief Initializes the complete system including hardware, configuration, and tasks.
 * @return true if initialization finished without critical errors, otherwise false.
 */
bool initializeSystem();

void measurementStartStop();

/**
 * @brief Initializes all hardware (I2C, SD, RTC, BMS).
 * @return true if all hardware components were initialized successfully, otherwise false.
 */
bool initializeHardware();

/**
 * @brief Loads and parses all required JSON configuration files.
 * @return true if all files were loaded successfully, otherwise false.
 */
bool loadConfiguration();

/**
 * @brief Synchronizes the system time via WiFi/NTP and updates the RTC.
 * @return true if synchronization was successful, otherwise false.
 */
bool synchronizeTime();

/**
 * @brief Turns WiFi off completely to save power.
 * @return void
 */
void disableWiFi();

/**
 * @brief Checks critical system components for proper operation.
 * @return true if all critical components operate without errors, otherwise false.
 */
bool verifySystemHealth();

void scheduleRtcAlarms(int32_t startEpoch, int32_t stopEpoch);

void checkCriticalBatteryVoltage();

#endif
