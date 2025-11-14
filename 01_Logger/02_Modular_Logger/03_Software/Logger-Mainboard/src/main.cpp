/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Main application loop and system initialization
 */

#include <Arduino.h>

#include "BMS.h"
#include "DS3231TimeNtp.h"
#include "DebuggingSDLog.h"
#include "DeepSleep.h"
#include "Led.h"
#include "MQTTManager.h"
#include "SDCard.h"
#include "SensorManagement.h"
#include "SystemVariables.h"
#include "Utility.h"

void setup()
{
  Serial.begin(115200);
  enable3V3(); // Enables power supply.
  initializeLogger();
  initBmsAndRtc();
  initializeSdCard();
  //programBms(); //* Optional (should only be activated if you want to program BMS, reason: BMS and RTC would use the interface at the same time!)
  performFirstBootOperations();
}

void loop()
{
  askForConfig();                      //* Configuration request
  checkWetSensorThreshold();           //* Underwater/surface water detection and operations
  manageBatteryCharging();             //* Battery management
  handleSensorError(30);               //* Sensor and config error detection
  processAndTransmitMeasurementData(); //* MQTT, data processing and transmission

  //* The variable totalElapsedTime += difftime(getCurrentTimeFromRTC(), currentTimeNow); is used to,
  //* update the total time since the start of the program by adding the elapsed
  //* time elapsed since the last run of the loop is added to the previous total time.
  totalElapsedTime += difftime(getCurrentTimeFromRTC(), currentTimeNow);

  //* Execution of the various periodic actions
  wetDetPeriodeFunktion(wet_det_periode);
  statusUploadPeriodeFunktion(status_upload_periode);
  configUpdatePeriodeFunktion(config_update_periode);
  dataUploadRetryPeriodeFunktion(data_upload_retry_periode);

  //* Determines the largest number from a series of time periods to restart the time loop
  resetTimePeriodeLoop(config_update_periode, status_upload_periode, wet_det_periode, data_upload_retry_periode);

  //* Calculation of the minimum waiting time
  minTimeUntilNextFunction = calculateShortestWaitTime(totalElapsedTime, lastConfigUpdateTime, lastStatusUploadTime, lastWetDetectionUploadTime, lastDataUploadRetryTime, isDataUploadRetryEnabled, config_update_periode, status_upload_periode, wet_det_periode, data_upload_retry_periode);

  //* Deep Sleep
  //* The variable currentTimeNow = getCurrentTimeFromRTC(); updates the variable currentTimeNow with the
  //* current time from the real-time clock (RTC) directly before the ESP32 goes into deep sleep.
  enableExternalWakeup(20); // activate Logger if power supply connection
  enableExternalWakeup(17); // activate Logger if reed connection
  interfaceSleep();
  currentTimeNow = getCurrentTimeFromRTC();
  esp_sleep_enable_timer_wakeup((minTimeUntilNextFunction) * 1000000);
  esp_deep_sleep_start();
}