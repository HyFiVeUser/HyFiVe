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
#include "LedManager.h"
#include "MQTTManager.h"
#include "SDCard.h"
#include "SensorManagement.h"
#include "SystemVariables.h"
#include "Utility.h"
#include "WifiNetwork.h"

unsigned long tLoopStart = 0;
unsigned long tLoopEnd   = 0;

void setup()
{
  Serial.begin(115200);
  ledInit();
  reedMonitorInit();
  enable3V3(); // Enables power supply.
  initializeLogger();
  initBmsAndRtc();
  initializeSdCard();
  // programBms(); //* Optional (should only be activated if you want to program BMS, reason: BMS and RTC would use the interface at the same time!)
  performFirstBootOperations();
  Serial.println("-------------------------------------------------0    1");
  if (statusDeepSleep)
  {
    inactiveMeasurement();
    Serial.println("-------------------------------------------------0    2");
  }

  getInactiveMeasurement();
  Serial.println("-------------------------------------------------0    3");
}

void loop()
{
  tLoopStart = millis();

  Serial.println("-------------------------------------------------0    4");

  //* The variable totalElapsedTime += difftime(getCurrentTimeFromRTC(), currentTimeNow); is used to,
  //* update the total time since the start of the program by adding the elapsed
  //* time elapsed since the last run of the loop is added to the previous total time.
  totalElapsedTime += difftime(getCurrentTimeFromRTC(), currentTimeNow);
  Serial.println("-------------------------------------------------0    9");

  //* Execution of the various periodic actions
  wetDetPeriodeFunktion(wet_det_periode);
  Serial.println("-------------------------------------------------0    10");
  statusUploadPeriodeFunktion(status_upload_periode);
  Serial.println("-------------------------------------------------0    11");
  configUpdatePeriodeFunktion(config_update_periode);
  Serial.println("-------------------------------------------------0    12");
  dataUploadRetryPeriodeFunktion(data_upload_retry_periode);
  Serial.println("-------------------------------------------------0    13");

  Serial.println("-------------------------------------------------0    5");
  manageBatteryCharging(); //* Battery management
  Serial.println("-------------------------------------------------0    6");
  handleSensorError(30); //* Sensor and config error detection
  Serial.println("-------------------------------------------------0    7");
  processAndTransmitMeasurementData(); //* MQTT, data processing and transmission
  Serial.println("-------------------------------------------------0    8");

  //* Determines the largest number from a series of time periods to restart the time loop
  resetTimePeriodeLoop(config_update_periode, status_upload_periode, wet_det_periode, data_upload_retry_periode);
  Serial.println("-------------------------------------------------0    14");

  //* Calculation of the minimum waiting time
  minTimeUntilNextFunction = calculateShortestWaitTime(totalElapsedTime, lastConfigUpdateTime, lastStatusUploadTime, lastWetDetectionUploadTime, lastDataUploadRetryTime, isDataUploadRetryEnabled, config_update_periode, status_upload_periode, wet_det_periode, data_upload_retry_periode);
  Serial.println("-------------------------------------------------0    15");
  while (statusReedInput.load())
  {
    Serial.println("-------------------------------------------------0    16");
    delay(100);
  }
  //* Deep Sleep
  //* The variable currentTimeNow = getCurrentTimeFromRTC(); updates the variable currentTimeNow with the
  //* current time from the real-time clock (RTC) directly before the ESP32 goes into deep sleep.
  enableExternalWakeup(20); // activate Logger if power supply connection
  enableExternalWakeup(17); // activate Logger if reed connection
  Serial.println("-------------------------------------------------0    17");
  interfaceSleep();
  Serial.println("-------------------------------------------------0    18");
  currentTimeNow = getCurrentTimeFromRTC();
  esp_sleep_enable_timer_wakeup((minTimeUntilNextFunction) * 1000000);
  Serial.println("-------------------------------------------------0    19");
  Serial.println("Deep Sleep");
  wiFiDisconnect();
  Serial.flush();
  // esp_deep_sleep_start();
  esp_light_sleep_start();

  // tLoopEnd = millis();
  // unsigned long loopDiff = tLoopEnd - tLoopStart;
  //
  // if ((minTimeUntilNextFunction * 1000) <= loopDiff)
  // {
  //   Serial.println("-------------------------------------------------0    20");
  //   esp_sleep_enable_timer_wakeup(0);
  //   esp_light_sleep_start();
  // }
  // else
  // {
  //   uint32_t espSleepWakeupTime = (minTimeUntilNextFunction * 1000) - loopDiff;
  //   esp_sleep_enable_timer_wakeup(espSleepWakeupTime * 1000);
  //   Serial.println("-------------------------------------------------0    19");
  //   Serial.println("Deep Sleep");
  //   Serial.flush();
  //   // esp_deep_sleep_start();
  //   esp_light_sleep_start();
  // }
}