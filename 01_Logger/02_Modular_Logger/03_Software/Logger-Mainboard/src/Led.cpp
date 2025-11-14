/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: LED control and status indication
 */

#include "Led.h"
#include "SystemVariables.h"
#include "SensorManagement.h"


const uint32_t LedUltraShort = 100;      // 100 ms
const uint32_t LedShort = 500;           // 500 ms
const uint32_t LedLong = 2000;           // 2000 ms
const uint32_t LedBreak = 500;           // 500 ms
const uint32_t LedBreakUltraShort = 100; // 100 ms

void turnOnLed()
{
  digitalWrite(42, HIGH);
}

void turnOffLed()
{
  digitalWrite(42, LOW);
}

/**
 * @brief Resets the ESP32 and turns on the LED.
 */
void resetEsp32()
{
  pinMode(GPIO_NUM_42, OUTPUT);
  gpio_hold_dis(GPIO_NUM_42);
  turnOnLed();
}

/**
 * @brief Sets the reset ESP pin to LOW.
 */
void setResetEspLow()
{
  pinMode(GPIO_NUM_42, OUTPUT);
  gpio_hold_dis(GPIO_NUM_42);
  digitalWrite(42, LOW);
  gpio_hold_en(GPIO_NUM_42);
  gpio_deep_sleep_hold_en();
}

/**
 * @brief Blinks the LED a specified number of times with given on and off durations.
 * @param times Number of times to blink.
 * @param onTime Duration the LED stays on (in milliseconds).
 * @param offTime Duration the LED stays off (in milliseconds).
 */
void blinkLED(uint8_t times, uint32_t onTime, uint32_t offTime)
{
  pinMode(GPIO_NUM_42, OUTPUT);
  gpio_hold_dis(GPIO_NUM_42);

  for (int i = 0; i < times; i++)
  {
    turnOnLed();
    delay(onTime);
    turnOffLed();
    delay(offTime);
  }

  digitalWrite(42, LOW);
  gpio_hold_en(GPIO_NUM_42);
  gpio_deep_sleep_hold_en();
}

// Various LED pattern functions for different states
void firstBootLedStartLED()
{
  blinkLED(1, LedUltraShort, LedBreakUltraShort);
}

void bootFinishedLED()
{
  delay(500);
  blinkLED(1, LedLong, 0);
}

void batteryCompletelyChargedLED()
{
  delay(2000);
  blinkLED(5, 5000, 500);
  delay(2000);
}

void bootFinishedButNtpUpdateNotPossibleLED()
{
  delay(500);
  blinkLED(3, LedShort, LedBreak);
}

void connectionOfPowerSupplyBeginChargingOfBatteriesLED()
{
  blinkLED(1, LedShort, LedBreak);
}

void loggerDetectsEndOfDeploymentLED()
{
  blinkLED(3, 2000, 500);
}

void generalAlarmLed()
{
  interfaceSleep();
  processAndTransmitMeasurementData();
  while (1)
  {
    blinkLED(1, 10, 500);
  }
}

void batteryRemainingLowLED()
{
  delay(2000);
  blinkLED(1, 10, 500);
  delay(2000);
}

void loggerTransmittedMeasurementDataLED()
{
  blinkLED(1, 500, 500);
  blinkLED(1, 2000, 500);
  blinkLED(1, 500, 500);
  delay(1000);
}

/**
 * @brief Task function for LED control.
 */
void ledTask(void *pvParameters)
{
  while (true)
  {
    if (isfirstBootLed)
    {
      firstBootLedStartLED();
    }
  }
}

/**
 * @brief Task function for blinking LED during initial measurements.
 * @param pvParameters Pointer to the total measurement count.
 */
void blinkLEDForFirstMeasurements(void *pvParameters)
{
  totalMeasurementCount = *(int *)pvParameters;
  if (totalMeasurementCount <= maxMeasurementCountForLed)
  {
    pinMode(GPIO_NUM_42, OUTPUT);
    gpio_hold_dis(GPIO_NUM_42);

    turnOnLed();
    delay(250);
    ledOff.store(true);
    turnOffLed();

    digitalWrite(42, LOW);
    gpio_hold_en(GPIO_NUM_42);
    gpio_deep_sleep_hold_en();
  }
  vTaskDelete(NULL);
}

/**
 * @brief Initializes the first boot LED sequence.
 */
void firstBootLed()
{
  xTaskCreatePinnedToCore(ledTask, "ledTask", 10000, NULL, 1, NULL, 1); // Starte die LED-Funktionen auf dem zweiten Core
  isfirstBootLed = true;
}

/**
 * @brief Starts the LED blink task for initial measurements.
 */
void startLEDBlinkTaskForInitialMeasurements()
{
  TaskHandle_t ledTaskHandle = NULL;
  BaseType_t ledTaskCreated = pdFALSE;

  if (totalMeasurementCount <= maxMeasurementCountForLed)
  {
    ledTaskCreated = xTaskCreatePinnedToCore(blinkLEDForFirstMeasurements, "blinkLEDForFirstMeasurements", 1024, (void *)&totalMeasurementCount, 1, &ledTaskHandle, 1);
  }
  else
  {
    ledMeasurementsOff.store(true);
  }
}