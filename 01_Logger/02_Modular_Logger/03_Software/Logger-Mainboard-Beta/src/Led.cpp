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
#include "Utility.h"
#include "DeepSleep.h"
#include "NVSPreferences.h"

// ------------------------------------------------------------
//  Hardware / Basis
// ------------------------------------------------------------

int greenPin = -1;
int bluePin = -1;
int redPin = 42;

// Bei Common-Anode auf true setzen (Pegel werden invertiert)
const bool COMMON_ANODE = false;
inline int ON() { return COMMON_ANODE ? LOW : HIGH; }
inline int OFF() { return COMMON_ANODE ? HIGH : LOW; }

enum class LedHw
{
  RGB,
  RED_ONLY
};
static LedHw ledHw = LedHw::RED_ONLY;

// ------------------------------------------------------------
//  Low-Level: Setzen der LED-Farben
// ------------------------------------------------------------

static void setRGB(bool r, bool g, bool b)
{
  if (ledHw == LedHw::RED_ONLY)
  {
    bool anyOn = r || g || b;
    digitalWrite(redPin, anyOn ? ON() : OFF());
    return;
  }

  digitalWrite(redPin, r ? ON() : OFF());
  digitalWrite(greenPin, g ? ON() : OFF());
  digitalWrite(bluePin, b ? ON() : OFF());
}

static void allOff() { setRGB(false, false, false); }
static void red() { setRGB(true, false, false); }
static void green() { setRGB(false, true, false); }
static void blue() { setRGB(false, false, true); }
static void yellow() { setRGB(true, true, false); }
static void magenta() { setRGB(true, false, true); }
static void cyan() { setRGB(false, true, true); }
static void white() { setRGB(true, true, true); }

static void applyColorFromCode(uint8_t colorCode, void (*fallback)())
{
  switch (colorCode)
  {
  case 1:
    red();
    break;
  case 2:
    green();
    break;
  case 3:
    yellow();
    break;
  case 4:
    blue();
    break;
  case 5:
    magenta();
    break;
  case 6:
    cyan();
    break;
  case 7:
    white();
    break;
  default:
    fallback();
    break;
  }
}

void normalUsage()
{
  uint8_t normalColorCode = (ledColorConfig / 100) % 10;
  applyColorFromCode(normalColorCode, green);
}
void interaction()
{
  uint8_t interactionColorCode = (ledColorConfig / 10) % 10;
  applyColorFromCode(interactionColorCode, yellow);
}
void errors()
{
  uint8_t errorColorCode = ledColorConfig % 10;
  applyColorFromCode(errorColorCode, red);
}

// ------------------------------------------------------------
//  Hardware-Variante initialisieren
// ------------------------------------------------------------

static void hwVariante()
{
  if (readPrefs() == "000001") // if ((readPrefs() == "000001") && (rgbVersion == 1))
  {
    greenPin = 37;
    bluePin = 36;
    redPin = 42;
    ledHw = LedHw::RGB;
  }
  else
  {
    greenPin = -1;
    bluePin = -1;
    redPin = 42;
    ledHw = LedHw::RED_ONLY;
  }
}

static void ledPMode()
{
  if (redPin >= 0)
    pinMode(redPin, OUTPUT);
  if (greenPin >= 0)
    pinMode(greenPin, OUTPUT);
  if (bluePin >= 0)
    pinMode(bluePin, OUTPUT);

  allOff();
}

// ------------------------------------------------------------
//  Timings (wie bei dir)
// ------------------------------------------------------------

static const uint32_t LedUltraShort = 100;      // ms
static const uint32_t LedShort = 500;           // ms
static const uint32_t LedLong = 2000;           // ms
static const uint32_t LedBreak = 500;           // ms
static const uint32_t LedBreakUltraShort = 100; // ms

static const uint32_t ledSignalLoggerDetectsBeginOfDeployment = 60000; // ms
static const uint32_t ledSignalShort = 200;                            // ms
static const uint32_t ledSignalLong = 600;                             // ms
static const uint32_t ledSignalBreakBetweenLetters = 200;              // ms
static const uint32_t ledSignalPermanent = 2000;                       // ms
static const uint32_t ledSignalPauseBetweenSignals = 800;              // ms

// ------------------------------------------------------------
//  Interne Task-Steuerung
// ------------------------------------------------------------

static TaskHandle_t s_ledTaskHandle = nullptr;
static QueueHandle_t s_ledQueue = nullptr;

// Hilfsfunktion: Wartezeit, aber abbrechbar durch neuen Mode
static bool waitOrNewMode(uint32_t ms, LedMode &newModeOut)
{
  if (!s_ledQueue)
  {
    vTaskDelay(pdMS_TO_TICKS(ms));
    return false;
  }

  if (xQueueReceive(s_ledQueue, &newModeOut, pdMS_TO_TICKS(ms)) == pdTRUE)
    return true;

  return false;
}

// ------------------------------------------------------------
//  Pattern-Runner: gibt den nächsten Mode zurück
//  - entweder Off (wenn fertig)
//  - oder einen neuen Mode, wenn währenddessen umgeschaltet wurde
// ------------------------------------------------------------

static LedMode runMode(LedMode mode)
{
  LedMode incoming = LedMode::Off;

  switch (mode)
  {
  // -----------------------
  // Boot/Status one-shot
  // -----------------------
  case LedMode::loggerBusyBackgroundProcess:
  {
    Serial.println("loggerBusyBackgroundProcess");

    while (1)
    {
      interaction();
      if (waitOrNewMode(ledSignalLong, incoming))
        return incoming;
      allOff();
      if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
        return incoming;
      interaction();
      if (waitOrNewMode(ledSignalShort, incoming))
        return incoming;
      allOff();
      if (waitOrNewMode(ledSignalPauseBetweenSignals, incoming))
        return incoming;

      if (!statusIsLoggerBusy.load())
      {
        break;
      }
    }

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::magnetDetected:
  {
    Serial.println("magnetDetected");
    normalUsage();
    if (waitOrNewMode(ledSignalShort, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalPauseBetweenSignals, incoming))
      return incoming;

    allOff();
    return LedMode::Off;
  }

  case LedMode::loggerActive:
  {
    Serial.println("loggerActive");
    normalUsage();
    if (waitOrNewMode(ledSignalShort, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalPauseBetweenSignals, incoming))
      return incoming;

    allOff();
    return LedMode::Off;
  }

  case LedMode::chargingComplete:
  {
    Serial.println("chargingComplete");
    normalUsage();
    if (waitOrNewMode(ledSignalPermanent, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalPauseBetweenSignals, incoming))
      return incoming;

    allOff();
    return LedMode::Off;
  }

  case LedMode::batteryCharging:
  {
    Serial.println("batteryCharging");
    interaction();
    delay(ledSignalShort);
    allOff();
    delay(ledSignalPauseBetweenSignals);

    allOff();
    return LedMode::Off;
  }

  case LedMode::batteryLow:
  {
    Serial.println("batteryLow");
    interaction();
    delay(ledSignalLong);
    allOff();
    delay(ledSignalBreakBetweenLetters);

    interaction();
    delay(ledSignalLong);
    allOff();
    delay(ledSignalBreakBetweenLetters);

    interaction();
    delay(ledSignalLong);
    allOff();
    delay(ledSignalPauseBetweenSignals);

    allOff();
    return LedMode::Off;
  }

  case LedMode::batterySuperlow:
  {
    Serial.println("batterySuperlow");
    errors();
    delay(ledSignalLong);
    allOff();
    delay(ledSignalBreakBetweenLetters);

    errors();
    delay(ledSignalLong);
    allOff();
    delay(ledSignalBreakBetweenLetters);

    errors();
    delay(ledSignalLong);
    allOff();
    delay(ledSignalPauseBetweenSignals);

    allOff();
    return LedMode::Off;
  }

  case LedMode::ntpUpdateFailed:
  {
    Serial.println("ntpUpdateFailed");

    for (int i = 0; i < 3; i++)
    {
      errors();
      delay(ledSignalShort);
      allOff();
      delay(ledSignalBreakBetweenLetters);

      errors();
      delay(ledSignalShort);
      allOff();
      delay(ledSignalBreakBetweenLetters);

      errors();
      delay(ledSignalShort);
      allOff();
      delay(ledSignalPauseBetweenSignals);
    }

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::configRejected:
  {
    Serial.println("configRejected");

    for (int i = 0; i < 3; i++)
    {
      errors();
      delay(ledSignalLong);
      allOff();
      delay(ledSignalBreakBetweenLetters);

      errors();
      delay(ledSignalLong);
      allOff();
      delay(ledSignalPauseBetweenSignals);
    }

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::noConnectionToDeckbox:
  {
    Serial.println("noConnectionToDeckbox");

    errors();
    delay(ledSignalShort);
    allOff();
    delay(ledSignalBreakBetweenLetters);

    errors();
    delay(ledSignalLong);
    allOff();
    delay(ledSignalBreakBetweenLetters);

    errors();
    delay(ledSignalShort);
    allOff();
    delay(ledSignalPauseBetweenSignals);

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::updateBootComplete:
  {
    Serial.println("updateBootComplete");

    interaction();
    delay(ledSignalPermanent);
    allOff();
    delay(ledSignalPauseBetweenSignals);

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::startConfigUpdate:
  {
    Serial.println("startConfigUpdate");

    interaction();
    delay(ledSignalShort);
    allOff();
    delay(ledSignalBreakBetweenLetters);

    interaction();
    delay(ledSignalShort);
    allOff();
    delay(ledSignalPauseBetweenSignals);

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::startReboot:
  {
    Serial.println("startReboot");

    interaction();
    delay(ledSignalLong);
    allOff();
    delay(ledSignalBreakBetweenLetters);

    interaction();
    delay(ledSignalLong);
    allOff();
    delay(ledSignalPauseBetweenSignals);

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::transmissionComplete:
  {
    Serial.println("transmissionComplete");

    normalUsage();
    if (waitOrNewMode(ledSignalPermanent, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalPauseBetweenSignals, incoming))
      return incoming;

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  // -----------------------
  // Sensor-spezifisch
  // -----------------------
  case LedMode::loggerDetectsBeginOfDeployment:
  {
    sensorStartDone = false;
    const uint32_t blinkDurationMs = ledSignalLoggerDetectsBeginOfDeployment;
    const uint32_t firstOnDurationMs = ledSignalShort;
    const uint32_t firstOffDurationMs = ledSignalBreakBetweenLetters;

    TickType_t start = xTaskGetTickCount();
    TickType_t end = start + pdMS_TO_TICKS(blinkDurationMs);

    while (xTaskGetTickCount() < end)
    {
      normalUsage();
      if (waitOrNewMode(firstOnDurationMs, incoming))
      {
        allOff();
        sensorStartDone = true; // <-- wichtig!
        return incoming;
      }

      allOff();
      if (waitOrNewMode(firstOffDurationMs, incoming))
      {
        sensorStartDone = true; // <-- wichtig!
        return incoming;
      }
    }

    allOff();
    sensorStartDone = true;
    return LedMode::Off;
  }

  case LedMode::duringDeployment:
    ledMeasurementsOff.store(false);

    normalUsage();
    if (waitOrNewMode(ledSignalShort, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
      return incoming;

    ledMeasurementsOff.store(true);

    return LedMode::Off;

  case LedMode::loggerDetectsEndOfDeployment:
  {
    ledMeasurementsOff.store(false);

    normalUsage();
    if (waitOrNewMode(ledSignalLong, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
      return incoming;
    normalUsage();
    if (waitOrNewMode(ledSignalLong, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
      return incoming;

    ledMeasurementsOff.store(true);

    return LedMode::Off;
  }

  // -----------------------
  default:
    allOff();
    return LedMode::Off;
  }
}

// ------------------------------------------------------------
//  Der eine LED-Task
// ------------------------------------------------------------

static void ledTask(void *pvParameters)
{
  LedMode mode = LedMode::Off;

  while (true)
  {
    if (mode == LedMode::Off)
    {
      allOff();
      // Warte auf neuen Mode
      xQueueReceive(s_ledQueue, &mode, portMAX_DELAY);
      continue;
    }

    // Mode ausführen (one-shot oder blockierend)
    mode = runMode(mode);
  }
}

// ------------------------------------------------------------
//  Init + Public Control
// ------------------------------------------------------------

void ledInit()
{
  hwVariante();
  ledPMode();

  if (!s_ledQueue)
  {
    // Länge 1 reicht: wir wollen immer nur den neuesten Mode
    s_ledQueue = xQueueCreate(1, sizeof(LedMode));
  }

  if (!s_ledTaskHandle)
  {
    xTaskCreatePinnedToCore(
        ledTask,
        "LedServiceTask",
        4096,
        nullptr,
        1,
        &s_ledTaskHandle,
        1);
  }

  // Startzustand
  LedMode off = LedMode::Off;
  xQueueOverwrite(s_ledQueue, &off);
}

/**
 * DER eine öffentliche Einstieg:
 * - Übergib den gewünschten Modus
 * - LedMode::Off stoppt alles sofort
 */
void ledControl(LedMode mode)
{
  if (!s_ledQueue)
    return;
  xQueueOverwrite(s_ledQueue, &mode);
}

void generalError()
{
  while (true)
  {
    for (int i = 0; i < 3; i++)
    {
      errors();
      delay(ledSignalShort);
      allOff();
      delay(ledSignalBreakBetweenLetters);
    }
    for (int i = 0; i < 3; i++)
    {
      errors();
      delay(ledSignalLong);
      allOff();
      delay(ledSignalBreakBetweenLetters);
    }
    for (int i = 0; i < 3; i++)
    {
      errors();
      delay(ledSignalShort);
      allOff();
      delay(ledSignalBreakBetweenLetters);
    }
  }
}

/**
 * @brief Sets the reset ESP pin to LOW.
 */
void setResetEspLow()
{
  allOff();
}