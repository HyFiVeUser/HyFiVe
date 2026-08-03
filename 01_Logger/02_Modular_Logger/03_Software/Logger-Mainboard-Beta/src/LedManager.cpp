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

#include <atomic>

#include "DeepSleep.h"
#include "LedManager.h"
#include "NVSPreferences.h"
#include "SensorManagement.h"
#include "SystemVariables.h"
#include "Utility.h"

int greenPin = -1;
int bluePin  = -1;
int redPin   = 42;

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

String printBits(uint32_t value)
{
  String result = "";

  for (int i = 18; i >= 0; i--)
  {
    result += String((value >> i) & 1);
  }

  return result;
}

void resetLedBitMask() { ledBitMask = 0; }

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
    bluePin  = 36;
    redPin   = 42;
    ledHw    = LedHw::RGB;
  }
  else
  {
    greenPin = -1;
    bluePin  = -1;
    redPin   = 42;
    ledHw    = LedHw::RED_ONLY;
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
//  Timings
// ------------------------------------------------------------

static const uint32_t ledSignalLoggerDetectsBeginOfDeployment = 30000; // ms
static const uint32_t ledSignalShort                          = 200;   // ms
static const uint32_t ledSignalLong                           = 600;   // ms
static const uint32_t ledSignalBreakBetweenLetters            = 200;   // ms
static const uint32_t ledSignalPermanent                      = 2000;  // ms
static const uint32_t ledSignalPauseBetweenSignals            = 3000;  // ms
static const uint32_t ledSignalPauseBetweenSignalsEvent1to5   = 800;   // ms

// ------------------------------------------------------------
//  Interne Task-Steuerung
// ------------------------------------------------------------

static TaskHandle_t s_ledTaskHandle    = nullptr;
static QueueHandle_t s_ledQueue        = nullptr;
static bool s_ledTaskSuspendedForError = false;
static std::atomic<bool> s_fatalErrorActive{false};
static std::atomic<bool> s_magnetDetectedDuringFatalError{false};
static std::atomic<LedMode> s_activeMode{LedMode::Off};
static std::atomic<LedMode> s_queuedMode{LedMode::Off};
static portMUX_TYPE s_ledControlMux = portMUX_INITIALIZER_UNLOCKED;

static void markModeDequeued(LedMode mode)
{
  portENTER_CRITICAL(&s_ledControlMux);
  if (uxQueueMessagesWaiting(s_ledQueue) == 0 && s_queuedMode.load() == mode)
    s_queuedMode.store(LedMode::Off);
  portEXIT_CRITICAL(&s_ledControlMux);
}

static void showMagnetDetectedDuringFatalError()
{
  Serial.println("magnetDetected (fatalError priority override)");
  normalUsage();
  delay(ledSignalShort);
  allOff();
  delay(ledSignalPauseBetweenSignalsEvent1to5);
}

// Keep the fatal-error pattern interruptible so magnetDetected can take priority.
static void waitDuringFatalError(uint32_t ms)
{
  const uint32_t startedAt = millis();

  while (millis() - startedAt < ms)
  {
    if (s_magnetDetectedDuringFatalError.exchange(false))
    {
      showMagnetDetectedDuringFatalError();
      continue;
    }

    const uint32_t elapsed = millis() - startedAt;
    if (elapsed >= ms)
      break;

    const uint32_t remaining = ms - elapsed;
    delay(remaining < 10 ? remaining : 10);
  }
}

// Hilfsfunktion: Wartezeit, aber abbrechbar durch neuen Mode
static bool waitOrNewMode(uint32_t ms, LedMode &newModeOut)
{
  if (!s_ledQueue)
  {
    vTaskDelay(pdMS_TO_TICKS(ms));
    return false;
  }

  if (xQueueReceive(s_ledQueue, &newModeOut, pdMS_TO_TICKS(ms)) == pdTRUE)
  {
    markModeDequeued(newModeOut);
    return true;
  }

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
    delay(ledSignalShort);
    allOff();
    delay(ledSignalPauseBetweenSignalsEvent1to5);

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
    if (waitOrNewMode(ledSignalPauseBetweenSignalsEvent1to5, incoming))
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

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::batteryCharging:
  {
    Serial.println("batteryCharging");
    interaction();
    if (waitOrNewMode(ledSignalShort, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalPauseBetweenSignals, incoming))
      return incoming;

    allOff();
    return LedMode::Off;
  }

  case LedMode::batteryLow:
  {
    Serial.println("batteryLow");
    interaction();
    if (waitOrNewMode(ledSignalLong, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
      return incoming;

    interaction();
    if (waitOrNewMode(ledSignalLong, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
      return incoming;

    interaction();
    if (waitOrNewMode(ledSignalLong, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalPauseBetweenSignals, incoming))
      return incoming;

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::batterySuperlow:
  {
    Serial.println("batterySuperlow");
    errors();
    if (waitOrNewMode(ledSignalLong, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
      return incoming;

    errors();
    if (waitOrNewMode(ledSignalLong, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
      return incoming;

    errors();
    if (waitOrNewMode(ledSignalLong, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalPauseBetweenSignals, incoming))
      return incoming;

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::ntpUpdateFailed:
  {
    Serial.println("ntpUpdateFailed");

    for (int i = 0; i < 3; i++)
    {
      errors();
      if (waitOrNewMode(ledSignalShort, incoming))
        return incoming;
      allOff();
      if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
        return incoming;

      errors();
      if (waitOrNewMode(ledSignalShort, incoming))
        return incoming;
      allOff();
      if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
        return incoming;

      errors();
      if (waitOrNewMode(ledSignalShort, incoming))
        return incoming;
      allOff();
      if (waitOrNewMode(ledSignalPauseBetweenSignals, incoming))
        return incoming;
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
      if (waitOrNewMode(ledSignalLong, incoming))
        return incoming;
      allOff();
      if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
        return incoming;

      errors();
      if (waitOrNewMode(ledSignalLong, incoming))
        return incoming;
      allOff();
      if (waitOrNewMode(ledSignalPauseBetweenSignals, incoming))
        return incoming;
    }

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::noConnectionToDeckbox:
  {
    Serial.println("noConnectionToDeckbox");

    errors();
    if (waitOrNewMode(ledSignalShort, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
      return incoming;

    errors();
    if (waitOrNewMode(ledSignalLong, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
      return incoming;

    errors();
    if (waitOrNewMode(ledSignalShort, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalPauseBetweenSignals, incoming))
      return incoming;

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::skipSensorError:
  {
    Serial.println("skipSensorError");

    errors();
    if (waitOrNewMode(ledSignalShort, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
      return incoming;

    errors();
    if (waitOrNewMode(ledSignalShort, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
      return incoming;

    errors();
    if (waitOrNewMode(ledSignalLong, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
      return incoming;

    errors();
    if (waitOrNewMode(ledSignalLong, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalPauseBetweenSignals, incoming))
      return incoming;

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::updateBootComplete:
  {
    Serial.println("updateBootComplete");

    interaction();
    if (waitOrNewMode(ledSignalPermanent, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalPauseBetweenSignals, incoming))
      return incoming;

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::startConfigUpdate:
  {
    Serial.println("startConfigUpdate");

    interaction();
    if (waitOrNewMode(ledSignalShort, incoming))
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

    statusLED = true;

    allOff();
    return LedMode::Off;
  }

  case LedMode::startReboot:
  {
    Serial.println("startReboot");

    interaction();
    if (waitOrNewMode(ledSignalLong, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
      return incoming;

    interaction();
    if (waitOrNewMode(ledSignalLong, incoming))
      return incoming;
    allOff();
    if (waitOrNewMode(ledSignalPauseBetweenSignals, incoming))
      return incoming;

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
    sensorStartDone                   = false;
    const uint32_t blinkDurationMs    = ledSignalLoggerDetectsBeginOfDeployment;
    const uint32_t firstOnDurationMs  = ledSignalShort;
    const uint32_t firstOffDurationMs = ledSignalBreakBetweenLetters;

    TickType_t start = xTaskGetTickCount();
    TickType_t end   = start + pdMS_TO_TICKS(blinkDurationMs);

    while (xTaskGetTickCount() < end)
    {
      if (!skipSensor)
      {
        normalUsage();
        if (waitOrNewMode(firstOnDurationMs, incoming))
        {
          allOff();
          sensorStartDone = true;
          return incoming;
        }
      }
      else
      {
        {
          errors();

          Serial.println("Event: 16	skip sensor error");
          statusLED = false;
          ledBitMask |= 0b0000000000000001000;

          if (waitOrNewMode(firstOnDurationMs, incoming))
          {
            allOff();
            sensorStartDone = true;
            return incoming;
          }
        }
      }

      allOff();
      if (waitOrNewMode(firstOffDurationMs, incoming))
      {
        sensorStartDone = true;
        return incoming;
      }
    }

    allOff();
    sensorStartDone = true;
    return LedMode::Off;
  }

  case LedMode::duringDeployment:
    ledMeasurementsOff.store(false);

    if (!skipSensor)
    {
      normalUsage();
      if (waitOrNewMode(ledSignalShort, incoming))
        return incoming;
      allOff();
      if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
        return incoming;
    }
    else
    {
      errors();

      Serial.println("Event: 16	skip sensor error");
      statusLED = false;
      ledBitMask |= 0b0000000000000001000;

      if (waitOrNewMode(ledSignalShort, incoming))
        return incoming;
      allOff();
      if (waitOrNewMode(ledSignalBreakBetweenLetters, incoming))
        return incoming;
    }

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
      markModeDequeued(mode);
    }

    // Mode ausführen (one-shot oder blockierend)
    s_activeMode.store(mode);
    const LedMode nextMode = runMode(mode);
    s_activeMode.store(nextMode);
    mode = nextMode;
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
    xTaskCreatePinnedToCore(ledTask, "LedServiceTask", 4096, nullptr, 1, &s_ledTaskHandle, 1);
  }

  // Startzustand
  LedMode off = LedMode::Off;
  s_activeMode.store(LedMode::Off);
  s_queuedMode.store(LedMode::Off);
  xQueueOverwrite(s_ledQueue, &off);
}

/**
 * DER eine öffentliche Einstieg:
 * - Übergib den gewünschten Modus
 * - LedMode::Off stoppt alles sofort
 */
void ledControl(LedMode mode)
{
  if (mode == LedMode::magnetDetected && s_fatalErrorActive.load())
  {
    s_magnetDetectedDuringFatalError.store(true);
    return;
  }

  if (!s_ledQueue)
    return;

  portENTER_CRITICAL(&s_ledControlMux);

  const LedMode activeMode        = s_activeMode.load();
  const LedMode queuedMode        = s_queuedMode.load();
  const uint8_t requestedPriority = ledModePriority(mode);
  const uint8_t activePriority    = activeMode == LedMode::Off ? 0 : ledModePriority(activeMode);
  const uint8_t queuedPriority    = queuedMode == LedMode::Off ? 0 : ledModePriority(queuedMode);

  // Lower-priority requests must neither interrupt the running pattern nor
  // replace a more important request that is already waiting in the queue.
  // Modes with equal priority retain the previous "latest request wins"
  // behaviour.
  if (mode != LedMode::Off && (requestedPriority < activePriority || requestedPriority < queuedPriority))
  {
    portEXIT_CRITICAL(&s_ledControlMux);
    return;
  }

  s_queuedMode.store(mode);
  xQueueOverwrite(s_ledQueue, &mode);
  portEXIT_CRITICAL(&s_ledControlMux);
}

void fatalError()
{
  s_fatalErrorActive.store(true);

  if (s_ledTaskHandle && !s_ledTaskSuspendedForError)
  {
    vTaskSuspend(s_ledTaskHandle);
    s_ledTaskSuspendedForError = true;
  }

  for (int y = 0; y < 10; y++)
  {
    for (int i = 0; i < 3; i++)
    {
      errors();
      waitDuringFatalError(ledSignalShort);
      allOff();
      waitDuringFatalError(ledSignalBreakBetweenLetters);
    }
    for (int i = 0; i < 3; i++)
    {
      errors();
      waitDuringFatalError(ledSignalLong);
      allOff();
      waitDuringFatalError(ledSignalBreakBetweenLetters);
    }
    for (int i = 0; i < 3; i++)
    {
      errors();
      waitDuringFatalError(ledSignalShort);
      allOff();
      waitDuringFatalError(ledSignalBreakBetweenLetters);
    }
  }

  ++generalErrorCounter;

  if (generalErrorCounter >= 3)
  {
    generalErrorCounter = 0;
    interfaceSleep();
    delay(1000);
    enableExternalWakeup(20); // activate Logger if power supply connection
    enableExternalWakeup(17); // activate Logger if reed connection
    statusDeepSleep = true;
    Serial.println("Deep Sleep fatalError");

    Serial.println("Event: 17	fatal error");
    statusLED = false;
    ledBitMask |= 0b0000000000000000100;

    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    esp_deep_sleep_start();
  }

  ESP.restart();
}

/**
 * @brief Sets the reset ESP pin to LOW.
 */
void setResetEspLow() { allOff(); }
