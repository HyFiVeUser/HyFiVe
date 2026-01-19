/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Central log level configuration
 */
#include <Arduino.h>

#include "DebuggingSDSetLog.h"

void setLogLevel(LogCategory category, uint8_t level) { logSettings[category] = static_cast<LogLevel>(level); }

void applyLogLevelSettings()
{
  setLogLevel(LogCategoryGeneral, generalLogLevel);
  setLogLevel(LogCategorySensors, sensorsLogLevel);
  setLogLevel(LogCategoryUnderwater, underwaterLogLevel);
  setLogLevel(LogCategoryAboveWater, aboveWaterLogLevel);
  setLogLevel(LogCategoryBMS, bmsLogLevel);
  setLogLevel(LogCategoryCharger, chargerLogLevel);
  setLogLevel(LogCategoryWiFi, wiFiLogLevel);
  setLogLevel(LogCategoryMQTT, mqttLogLevel);
  setLogLevel(LogCategorySDCard, sdCardLogLevel);
  setLogLevel(LogCategoryRTC, rtcLogLevel);
  setLogLevel(LogCategoryPowerManagement, powerManagementLogLevel);
  setLogLevel(LogCategoryConfiguration, configurationLogLevel);
  setLogLevel(LogCategoryMeasurement, measurementLogLevel);
}