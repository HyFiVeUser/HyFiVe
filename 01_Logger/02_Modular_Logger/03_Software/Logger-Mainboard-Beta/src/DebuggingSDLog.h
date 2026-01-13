/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Debugging and SD card logging functionality
 */

#ifndef DEBUGGINGSDLOG_H
#define DEBUGGINGSDLOG_H

#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

#include "DS3231TimeNtp.h"
#include "Utility.h"
#include "loggerConfig.h"

// Log-Levels
enum LogLevel
{
  LogLevelDEBUG = 0,
  LogLevelINFO,
  LogLevelWARNING,
  LogLevelERROR
};

// Log-Kategorien
enum LogCategory
{
  LogCategoryGeneral = 0,
  LogCategorySensors,
  LogCategoryUnderwater,
  LogCategoryAboveWater,
  LogCategoryBMS,
  LogCategoryCharger,
  LogCategoryWiFi,
  LogCategoryMQTT,
  LogCategorySDCard,
  LogCategoryRTC,
  LogCategoryPowerManagement,
  LogCategoryConfiguration,
  LogCategoryError,
  LogCategoryDebug,
  LogCategoryMeasurement,
};

// External mapping of log categories to log levels
extern std::map<LogCategory, LogLevel> logSettings;

// Helper function to convert LogCategory to string
inline std::string LogCategoryToString(LogCategory category)
{
  switch (category)
  {
  case LogCategoryGeneral:
    return "General";
  case LogCategorySensors:
    return "Sensors";
  case LogCategoryUnderwater:
    return "Underwater";
  case LogCategoryAboveWater:
    return "AboveWater";
  case LogCategoryBMS:
    return "BMS";
  case LogCategoryCharger:
    return "Charger";
  case LogCategoryWiFi:
    return "WiFi";
  case LogCategoryMQTT:
    return "MQTT";
  case LogCategorySDCard:
    return "SDCard";
  case LogCategoryRTC:
    return "RTC";
  case LogCategoryPowerManagement:
    return "PowerManagement";
  case LogCategoryConfiguration:
    return "Configuration";
  case LogCategoryError:
    return "Error";
  case LogCategoryDebug:
    return "Debug";
  case LogCategoryMeasurement:
    return "Measurement";
  default:
    return "UnknownCategory";
  }
}

inline std::string LogLevelToString(LogLevel level)
{
  switch (level)
  {
  case LogLevelDEBUG:
    return "DEBUG";
  case LogLevelINFO:
    return "INFO";
  case LogLevelWARNING:
    return "WARNING";
  case LogLevelERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

template <typename T> inline std::string ToString(const T &value)
{
  std::ostringstream oss;
  oss << value;
  return oss.str();
}

// Overload for the Arduino `String` type
inline std::string ToString(const String &value) { return value.c_str(); }

template <typename... Args> void Log(LogCategory category, LogLevel level, const std::string &message, Args... args)
{
  if (logSettings.find(category) != logSettings.end() && level >= logSettings[category])
  {
    std::ostringstream logStream;

    // Get the current time as a string
    std::string currentTime = getLocalTimeAsStringLog().c_str();

    // Start the log stream with category and level
    logStream << "[Logger ID: " << ToString(configRTC.logger_id) << "];";
    logStream << "[" << LogCategoryToString(category) << "];";
    logStream << "[" << LogLevelToString(level) << "];";
    logStream << currentTime << ";";

    // Add the actual message
    logStream << message;

    using expander = int[];
    (void)expander{0, (void(logStream << ToString(args)), 0)...};

    std::string logMessage = logStream.str() + "\n";

    File file = SD.open("/log/log.txt", FILE_APPEND);
    if (!file)
    {
      Serial.println("Error opening the file /log/log.txt");
      moveFileToDestination("/log", "log.txt", "/backup/log_error", true);
      return;
    }

    // if (bytesWritten != logMessage.length()): It is checked whether the number of bytes written (bytesWritten)
    // matches the length of the log message (logMessage.length()).
    // If the number of bytes written does not match the length of the log message, this means
    // that an error occurred when writing to the file.
    size_t bytesWritten = file.print(logMessage.c_str());
    if (bytesWritten != logMessage.length())
    {
      Serial.println("Error when writing to the file /log/log.txt");
      moveFileToDestination("/log", "log.txt", "/backup/log_error", true);
    }

    file.close();

    Serial.print(logMessage.c_str());
  }
}

#endif
