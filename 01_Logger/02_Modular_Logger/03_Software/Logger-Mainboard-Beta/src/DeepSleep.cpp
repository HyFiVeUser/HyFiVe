/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Deep sleep management for power saving
 */

#include <Arduino.h>
#include "DS3231TimeNtp.h"
#include "DeepSleep.h"
#include "SystemVariables.h"
#include "SensorManagement.h"

#define uS_TO_S_FACTOR 1000000UL

/**
 * @brief Puts the ESP32 into deep sleep for a specified number of seconds.
 * @param sleepTimeSec The number of seconds to sleep.
 */
void espDeepSleepSec(uint32_t sleepTimeSec)
{
  esp_sleep_enable_timer_wakeup(sleepTimeSec * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
  //esp_light_sleep_start();
}

int64_t activePinMask = 0;
/**
 * @brief Enables an external wake-up source on a specific pin.
 * @param pin The GPIO pin number to use as a wake-up source.
 */
void enableExternalWakeup(uint8_t pin)
{
  pinMode(pin, INPUT);
  activePinMask |= (1ULL << pin);
  esp_sleep_enable_ext1_wakeup(activePinMask, ESP_EXT1_WAKEUP_ANY_LOW);
}

/**
 * @brief Disables a previously enabled wake-up pin.
 * @param pin The GPIO pin number to disable as a wake-up source.
 */
void disableWakeupPin(uint8_t pin)
{
  if (activePinMask & (1ULL << pin))
  {
    activePinMask &= ~(1ULL << pin);
    if (activePinMask != 0)
    {
      esp_sleep_enable_ext1_wakeup(activePinMask, ESP_EXT1_WAKEUP_ANY_LOW);
    }
    else
    {
      esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT1);
    }
  }
  else
  {
    Serial.printf("is not enabled", pin);
  }
}

void inactiveMeasurement()
{
  secInactivityTimeoutSec = inactivityTimeoutSec;

  unsigned long now = getCurrentTimeFromRTC();
  secInactivityTimeoutSec = now + secInactivityTimeoutSec;

  statusDeepSleep = false;
}

void getInactiveMeasurement()
{
  if (!isLoggerSubmerged)
  {
    unsigned long now = getCurrentTimeFromRTC();
    if (secInactivityTimeoutSec <= now)
    {
      interfaceSleep();
      Serial.println("InactiveMeasurement time >15min go DeepSleep");
      enableExternalWakeup(20); // activate Logger if power supply connection
      enableExternalWakeup(17); // activate Logger if reed connection
      statusDeepSleep = true;
      esp_deep_sleep_start();
    }
    else
    {
      Serial.println("secInactivityTimeoutSec: now: ");
      Serial.println(secInactivityTimeoutSec);
      Serial.println(now);
    }
  }
  else
  {
    Serial.println("stop getInactiveMeasurement");
  }
}