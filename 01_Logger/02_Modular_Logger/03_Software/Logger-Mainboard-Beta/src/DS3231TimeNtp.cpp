/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: RTC (DS3231) and NTP time synchronization functions
 */

#include <Wire.h>

#include "DS3231TimeNtp.h"
#include "DebuggingSDLog.h"
#include "LedManager.h"
#include "RTClib.h"
#include "SystemVariables.h"

RTC_DS3231 rtcDS3231;

/**
 * @brief Initializes the RTC module.
 * @param wireInstance Pointer to the TwoWire instance for I2C communication.
 * @return true if initialization was successful, false otherwise.
 */
bool initRTC(TwoWire *wireInstance)
{
  if (!rtcDS3231.begin(wireInstance))
  {
    Log(LogCategoryRTC, LogLevelERROR, "DS3231 not found!");
    generalError();
  }

  return true;
}

/**
 * @brief Synchronizes the time with an NTP server.
 * @return true if synchronization was successful, false otherwise.
 */
bool synchronizeTimeWithNTP()
{
  if (hasWifiConnection)
  {
    const char *ntpServer        = "192.168.1.1";
    const long gmtOffset_sec     = 0;
    const int daylightOffset_sec = 0;

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    int attempts = 0;

    while (!getLocalTime(&timeinfo))
    {
      Serial.print(".");
      delay(500);
      attempts++;

      if (attempts >= 2)
      {
        Log(LogCategoryRTC, LogLevelERROR, "Time could not be synchronized.");
        isNtpSynchronized = false;
        return false;
      }
    }

    Log(LogCategoryRTC, LogLevelDEBUG, "Time synchronized.");
    rtcDS3231.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
    isNtpSynchronized = true;
    return true;
  }
  else
  {
    Log(LogCategoryRTC, LogLevelDEBUG, "Time could not be synchronized.");
    isNtpSynchronized = false;
    return false;
  }
}

/**
 * @brief Gets the current time from the RTC as a Unix timestamp.
 * @return unsigned long The current time as a Unix timestamp.
 */
unsigned long getCurrentTimeFromRTC()
{
  DateTime now = rtcDS3231.now();
  return now.unixtime();
}

/**
 * @brief Formats the current local time as an ISO 8601 string.
 * @return String The formatted time string.
 */
String formatLocalTimeAsISOString()
{
  DateTime now = rtcDS3231.now();
  char buffer[30];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02dZ", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  return String(buffer);
}

/**
 * @brief Gets the current local time as a string in "YYYYMMDDhhmmss" format.
 * @return String The formatted time string.
 */
String getLocalTimeAsStringBackup()
{
  DateTime now = rtcDS3231.now();
  char buffer[30];
  snprintf(buffer, sizeof(buffer), "%04d%02d%02d%02d%02d%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  return String(buffer);
}

/**
 * @brief Gets the current local time as a string in "YYYY.MM.DD;hh:mm:ss" format.
 * @return String The formatted time string.
 */
String getLocalTimeAsStringLog()
{
  DateTime now = rtcDS3231.now();
  char buffer[30];
  snprintf(buffer, sizeof(buffer), "%04d.%02d.%02d;%02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  return String(buffer);
}