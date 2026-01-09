#include "ble_time.h"

#include <NimBLECharacteristic.h>
#include <NimBLEDevice.h>
#include <NimBLEServer.h>
#include <cstdio>
#include <string>

#include "ble_server.h"
#include "ble_uuids.h"
#include "init.h"
#include "logging.h"
#include "rtc_second_tick.h"

namespace
{
    NimBLEService* g_timeService = nullptr;
    NimBLECharacteristic* g_timeCharacteristic = nullptr;
    bool g_serviceInitialized = false;

    String currentRtcTimestamp()
    {
        DateTime now = rtc.now();
        char buffer[25];
        snprintf(buffer,
                 sizeof(buffer),
                 "%04d-%02d-%02dT%02d:%02d:%02dZ",
                 now.year(),
                 now.month(),
                 now.day(),
                 now.hour(),
                 now.minute(),
                 now.second());
        return String(buffer);
    }

    bool parseIsoTimestamp(const std::string& value, DateTime& result)
    {
        if (value.size() < 19)
        {
            return false;
        }

        int year = 0;
        int month = 0;
        int day = 0;
        int hour = 0;
        int minute = 0;
        int second = 0;

        const int matched = std::sscanf(value.c_str(),
                                        "%4d-%2d-%2dT%2d:%2d:%2d",
                                        &year,
                                        &month,
                                        &day,
                                        &hour,
                                        &minute,
                                        &second);

        if (matched != 6)
        {
            return false;
        }

        if (year < 2000 || month < 1 || month > 12 || day < 1 || day > 31 ||
            hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 ||
            second > 59)
        {
            return false;
        }

        result = DateTime(year, month, day, hour, minute, second);
        return true;
    }

    class TimeCharacteristicCallbacks : public NimBLECharacteristicCallbacks
    {
    public:
        void onWrite(NimBLECharacteristic* characteristic,
                     NimBLEConnInfo& /*connInfo*/) override
        {
            const std::string value = characteristic->getValue();

            if (value.empty())
            {
                LOG_W("⌛ Empty time message received");
                return;
            }

            DateTime parsed;
            if (!parseIsoTimestamp(value, parsed))
            {
                LOG_E("Invalid time format: %s", value.c_str());
                return;
            }

            rtc.adjust(parsed);

            LOG_I("RTC updated to %04d-%02d-%02dT%02d:%02d:%02dZ",
                  parsed.year(),
                  parsed.month(),
                  parsed.day(),
                  parsed.hour(),
                  parsed.minute(),
                  parsed.second());
        }

        void onRead(NimBLECharacteristic* characteristic,
                    NimBLEConnInfo& /*connInfo*/) override
        {
            const String timestamp = currentRtcTimestamp();
            characteristic->setValue(timestamp.c_str());
        }
    };

    TimeCharacteristicCallbacks* g_timeCallbacks = nullptr;
}

bool initTimeService()
{
    if (g_serviceInitialized)
    {
        return true;
    }

    if (!bleServerInitialized || !bleServerInstance)
    {
        LOG_E("BLE server not available for Time Service");
        return false;
    }

    try
    {
        g_timeService = bleServerInstance->createService(TIME_SERVICE_UUID);
        if (!g_timeService)
        {
            LOG_E("Time Service could not be created");
            return false;
        }

        g_timeCharacteristic = g_timeService->createCharacteristic(
            TIME_CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE);

        if (!g_timeCharacteristic)
        {
            LOG_E("Time characteristic could not be created");
            return false;
        }

        g_timeCallbacks = new TimeCharacteristicCallbacks();
        g_timeCharacteristic->setCallbacks(g_timeCallbacks);

        const String initialTimestamp = currentRtcTimestamp();
        g_timeCharacteristic->setValue(initialTimestamp.c_str());

        g_serviceInitialized = true;
        LOG_I("Time Service initialized");
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_E("Time Service error: %s", e.what());
        return false;
    }
}

bool startTimeService()
{
    if (!g_serviceInitialized)
    {
        LOG_E("Time Service not initialized");
        return false;
    }

    try
    {
        g_timeService->start();
        LOG_I("Time Service started");
        return true;
    }
    catch (const std::exception& e)
    {
        LOG_E("Time Service start failed: %s", e.what());
        return false;
    }
}

void printTimeServiceStatus()
{
    LOG_I("\n=== TIME SERVICE STATUS ===");
    LOG_I("Service initialized: %s", g_serviceInitialized ? "✅" : "❌");
    if (g_serviceInitialized)
    {
        const String timestamp = currentRtcTimestamp();
        LOG_I("Current RTC: %s", timestamp.c_str());
    }
    LOG_I("============================\n");
}