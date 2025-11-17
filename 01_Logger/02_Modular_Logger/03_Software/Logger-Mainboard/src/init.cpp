#include "init.h"
#include "config.h"
#include "sensors.h"
#include "logging.h"
#include "ble_config.h"
#include "firmware_updater.h"

#include "deepSleepService.h"

// =========================
// GLOBAL OBJECTS
// =========================
RTC_DS3231 rtc;
TwoWire I2C_SENSORS = TwoWire(1);
uint64_t measurementCycle = 5; // Default: 5 seconds

// =========================
// MAIN INITIALIZATION
// =========================

bool initializeSystem()
{
    LOG_I(" Starting system initialization...");

    // 1. Initialize hardware (critical)
    if (!initializeHardware())
    {
        LOG_E("Hardware initialization failed!");
        return false;
    }

    // 2. Load configuration (critical for sensors)
    if (!loadConfiguration())
    {
        LOG_W("⚠ Configuration incomplete - using defaults");
        // Not critical - system can run with defaults
    }

    // 3. Optional: check for firmware update (not critical for start)
    if (!performFirmwareUpdateIfAvailable())
    {
        LOG_W("⚠ Firmware update could not be performed or was not necessary");
    }

    // 4. Synchronize time (important but not critical)
    //! if (!synchronizeTime())
    //! {
    //!     LOG_W("⚠ Time synchronization failed - using RTC time");
    //!     // Not critical - RTC time will be used
    //! }
    // 5. Check system health
    if (!verifySystemHealth())
    {
        LOG_E("System health check failed!");
        return false;
    }

    LOG_I("System initialization successful!");

    measurementStartStop();

    // 6. Initialize sensors
    LOG_I("🔍 Initializing sensors...");
    if (!initializeConfiguredSensors())
    {
        LOG_W("⚠ No sensors found - system running in demo mode");
    }

    disableWiFi();

    return true;
}

// =========================
// HARDWARE INITIALIZATION
// =========================

void measurementStartStop()
{
    // Plan RTC alarms according to configuration
    scheduleRtcAlarms(systemConfig.mainMcu.measurementStart,
                      systemConfig.mainMcu.measurementStop);
}

bool initializeHardware()
{
    LOG_I(" Initializing hardware...");

    // I2C for sensors
    I2C_SENSORS.begin(SENSOR_SDA_PIN, SENSOR_SCL_PIN, SENSOR_I2C_FREQUENCY);
    LOG_I("✓ Sensor I2C: SDA=%d, SCL=%d", SENSOR_SDA_PIN, SENSOR_SCL_PIN);

    // I2C for BMS and RTC (shared bus)
    Wire.begin(BMS_SDA_PIN, BMS_SCL_PIN, BMS_I2C_FREQUENCY);
    LOG_I("✓ BMS/RTC I2C: SDA=%d, SCL=%d", BMS_SDA_PIN, BMS_SCL_PIN);

    // Initialize RTC (critical)
    if (!rtc.begin())
    {
        LOG_E("RTC DS3231 not found!");
        return false;
    }
    LOG_I("✓ RTC DS3231 initialized");

    // Check BMS connection (critical for battery operation)
    Wire.beginTransmission(BQ40Z80_ADDR);
    if (Wire.endTransmission() != 0)
    {
        LOG_E("BMS BQ40Z80 not reachable!");
        return false;
    }
    LOG_I("✓ BMS BQ40Z80 connected");

    // SD card (critical for configuration)
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    constexpr uint8_t SD_INIT_MAX_RETRIES = 20;
    bool sdReady = false;

    for (uint8_t attempt = 0; attempt < SD_INIT_MAX_RETRIES; ++attempt)
    {
        if (SD.begin(SD_CS_PIN))
        {
            sdReady = true;
            break;
        }

        LOG_W("⚠ SD initialization failed (attempt %u/%u)", attempt + 1, SD_INIT_MAX_RETRIES);
        SD.end();
        digitalWrite(SD_CS_PIN, HIGH);
        delay(100);
    }

    if (!sdReady)
    {
        LOG_E("SD card not available!");
        return false;
    }
    LOG_I("✓ SD card available");
    errorSDCount = 0;

    // ESP32 sleep configuration
    esp_sleep_enable_ext0_wakeup(RTC_ALARM_PIN, 0); // if the signal is pulled to 0V the ESP32-S3 wakes up
    LOG_I("✓ Sleep mode configured");

    return true;
}

// =========================
// WIFI MANAGEMENT
// =========================

void disableWiFi()
{
    LOG_I("Turning off WiFi...");

    // Shut WiFi down completely
    WiFi.disconnect(true); // true = also delete WiFi configuration
    WiFi.mode(WIFI_OFF);

    LOG_I("✓ WiFi disabled");
}

// =========================
// TIME SYNCHRONIZATION
// =========================

bool synchronizeTime()
{
    LOG_I("Starting time synchronization...");

    if (systemConfig.mainMcu.wifiSsid.length() == 0)
    {
        LOG_W("⚠ No WiFi configuration - skipping NTP");
        return true;
    }

    // Connect WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(systemConfig.mainMcu.wifiSsid.c_str(),
               systemConfig.mainMcu.wifiPassword.c_str());

    LOG_I("Connecting to '%s'...", systemConfig.mainMcu.wifiSsid.c_str());

    uint32_t startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < WIFI_TIMEOUT_MS)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED)
    {
        LOG_E("WiFi connection failed");
        WiFi.disconnect();
        return false;
    }

    LOG_I("✓ WiFi connected");

    // Configure NTP
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);

    // Wait for NTP sync
    LOG_I("Waiting for NTP synchronization...");
    struct tm timeinfo;
    startTime = millis();

    while (!getLocalTime(&timeinfo) && (millis() - startTime) < NTP_TIMEOUT_MS)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (getLocalTime(&timeinfo))
    {
        // Transfer NTP time to the RTC
        DateTime ntpTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                         timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

        DateTime rtcTime = rtc.now();

        rtc.adjust(ntpTime);
        LOG_I("✓ RTC synchronized with NTP time");
    }
    else
    {
        LOG_E("NTP synchronization failed");
        WiFi.disconnect();
        return false;
    }

    // Disconnect WiFi to save power
    WiFi.disconnect();
    delay(1000);

    return true;
}

// =========================
// SYSTEM HEALTH CHECK
// =========================

namespace
{
    bool readBatteryVoltage(float &voltage)
    {
        Wire.beginTransmission(BQ40Z80_ADDR);
        Wire.write(VOLTAGE_REG);
        if (Wire.endTransmission() != 0)
        {
            return false;
        }

        if (Wire.requestFrom(static_cast<uint8_t>(BQ40Z80_ADDR), static_cast<uint8_t>(2)) != 2)
        {
            return false;
        }

        if (Wire.available() < 2)
        {
            return false;
        }

        uint8_t lsb = Wire.read();
        uint8_t msb = Wire.read();
        uint16_t voltage_raw = (msb << 8) | lsb;
        voltage = voltage_raw / 1000.0f;
        return true;
    }
}

void checkCriticalBatteryVoltage()
{
    float voltage = 0.0f;
    if (!readBatteryVoltage(voltage))
    {
        return;
    }

    if (voltage < 11.2f)
    {
        LOG_E("🔋 Battery: %.2fV [CRITICALLY LOW!]", voltage);
        esp32Error();
    }
}

bool verifySystemHealth()
{
    LOG_I("🔍 Checking system health...");

    float voltage = 0.0f;
    if (readBatteryVoltage(voltage))
    {
        if (voltage < 11.2f)
        {
            LOG_E("🔋 Battery: %.2fV [CRITICALLY LOW!]", voltage);
            esp32Error();
        }
        else if (voltage < 3.5f)
        {
            LOG_W("🔋 Battery: %.2fV [Low]", voltage);
        }
        else
        {
            LOG_I("🔋 Battery: %.2fV [OK]", voltage);
        }
    }

    // RTC Test
    DateTime now = rtc.now();
    if (now.year() < 2024)
    {
        LOG_E("RTC time invalid - check RTC battery");
        synchronizeTime();
        return true;
    }
    LOG_I("✓ RTC time: %04d-%02d-%02d %02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());

    LOG_I("System health OK");
    return true;
}

void scheduleRtcAlarms(int32_t startEpoch, int32_t stopEpoch)
{
    // INT mode (no square wave)
    rtc.writeSqwPinMode(DS3231_OFF);

    // Clear previous alarms
    rtc.clearAlarm(1);
    rtc.clearAlarm(2);

    bool alarmConfigured = false;

    if (startEpoch >= 0)
    {
        DateTime start(static_cast<uint32_t>(startEpoch));
        rtc.setAlarm1(start, DS3231_A1_Date);
        LOG_I("RTC Alarm1 (Wake): %04d-%02d-%02d %02d:%02d:%02d", start.year(), start.month(), start.day(), start.hour(), start.minute(), start.second());
        alarmConfigured = true;
    }
    else
    {
        LOG_I("RTC Alarm1 (Wake): disabled");
    }

    if (stopEpoch >= 0)
    {
        DateTime stop(static_cast<uint32_t>(stopEpoch));
        rtc.setAlarm2(stop, DS3231_A2_Date);
        LOG_I("RTC Alarm2 (Sleep): %04d-%02d-%02d %02d:%02d:%02d", stop.year(), stop.month(), stop.day(), stop.hour(), stop.minute(), stop.second());
        alarmConfigured = true;
    }
    else
    {
        LOG_I("RTC Alarm2 (Sleep): disabled");
    }

    if (!alarmConfigured)
    {
        LOG_W("No RTC alarms scheduled (measurementStart/Stop = -1)");
    }
}
