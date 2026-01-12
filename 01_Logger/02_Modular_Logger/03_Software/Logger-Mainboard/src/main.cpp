#include "init.h"
#include "sensors.h"
#include "power.h"
#include "ble_integration.h"
#include "ble_config.h"          // For bleConfigUploadActive
#include "ble_sd_card.h"         // For bleSDCardTransferActive
#include "measurement_storage.h" // For SD logging
#include "configuration_receiver.h"
#include "logging.h"
#include "bms_debug.h"
#include "led.h"
#include "deepSleepService.h"
#include "config.h"
#include "NVSPreferences.h"
#include "rtc_second_tick.h"

#include "BQ40Z80.h"
static BQ40Z80 gauge;

bool ledIsCommunicationActive = 0;
bool communicationCount = 0;
uint8_t transferActiveDone = 10;

const bool wokeByGPIO13 =
    (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1) &&
    (esp_sleep_get_ext1_wakeup_status() & (1ULL << GPIO_NUM_13));

void setup()
{
    Serial.begin(115200);

    setGPIO_NUM_13();

    ensureChargerPinsInit();

    readPrefs();
    enableCharger();

    if (!wokeByGPIO13)
    {
        if (isPG())
        {
            confirmRtcAlarmWake();
        }
    }
    else
    {
        LOG_I("wokeByGPIO13");
    }

    initStatusLed();

    LOG_I("=== OSBK ESP32-S3 MODULAR BLE ===");
    LOG_I("=== FW: 0.08 ===");

    // 1. Initialize hardware system
    if (!initializeSystem())
    {
        LOG_E("CRITICAL SYSTEM ERROR!");
        ++errorSDCount;

        if (errorSDCount == 2)
        {
            esp32Error();
        }

        esp_deep_sleep(1000000ULL); // 1sec
    }

    if (SD.exists(MEASUREMENTS_ROOT_DIR))
    {
        validateMeasurementIndex();
        delay(1000);
    }

    LOG_I("OSBK hardware system started!");

    logBmsSnapshot();

    initMeasurementStorage();

    // 2. Initialize BLE system (modular services)
    if (initializeBLE() && startBLE())
    {
        LOG_I("BLE integration successful (MODULAR)");
    }
    else
    {
        LOG_W("BLE integration failed");
    }

    LOG_I("Start loop");

    initGpio13Watch();

    ensureCurrentFile(rtc.now());

    sensorPrepDurationTime();

    updateRtcSecondTick(0);
}

void loop()
{
    monitorRtcAlarmForSleep();

    manageBatteryCharging();

    checkCriticalBatteryVoltage();

    pollConfigUploadTimeout();

    if (transferActive())
    {
        if (!ledIsCommunicationActive)
        {
            ledSetCommunicationActive(1);
            ledIsCommunicationActive = 1;
            communicationCount = 0;
        }
        notifyMeasurementPaused();
        transferActiveDone = 10;
        delay(100);
        return;
    }

    if (ledIsCommunicationActive)
    {
        ledSetCommunicationActive(0);
        ledIsCommunicationActive = 0;
        communicationCount = 0;
    }

    if (transferActiveDone == 0)
    {
        while (rtcSecondTickElapsed())
        {
            manageBatteryCharging();
            checkCriticalBatteryVoltage();

            if (transferActive())
            {
                notifyMeasurementPaused();
                transferActiveDone = 10;
                return;
            }

            // Read sensors
            readAndCalibrateSensors();

            // Store measurements
            storeSensorMeasurements();
            updateRtcSecondTick(measurementCycle);

            // BLE transmission
            transmitSensorData();
        }
    }
    else if (transferActiveDone > 0)
    {
        --transferActiveDone;
        delay(100);
    }
}
