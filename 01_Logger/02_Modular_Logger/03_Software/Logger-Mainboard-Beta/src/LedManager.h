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

#ifndef LED_H
#define LED_H

#include <cstdint>

enum class LedMode : uint32_t
{
  Off = 0,

  // normal usage
  magnetDetected,
  loggerActive,
  loggerDetectsBeginOfDeployment,
  duringDeployment,
  loggerDetectsEndOfDeployment,
  transmissionComplete,
  chargingComplete,

  // interaction
  batteryCharging,
  startConfigUpdate,
  startReboot,
  loggerBusyBackgroundProcess,
  updateBootComplete,
  batteryLow,

  // Errors
  batterySuperlow,
  noConnectionToDeckbox,
  skipSensorError,
  ntpUpdateFailed,
  configRejected,
};

/**
 * Priority of an LED mode.

 */
constexpr uint8_t ledModePriority(LedMode mode)
{
  switch (mode)
  {
  case LedMode::Off:
    return 255; // Explicitly switching the LED off must always be possible.

  // normal usage
  case LedMode::magnetDetected:
    return 19;
  case LedMode::loggerActive:
    return 18;
  case LedMode::loggerDetectsBeginOfDeployment:
    return 17;
  case LedMode::duringDeployment:
    return 16;
  case LedMode::loggerDetectsEndOfDeployment:
    return 15;
  case LedMode::transmissionComplete:
    return 14;
  case LedMode::chargingComplete:
    return 13;

  // interaction
  case LedMode::batteryCharging:
    return 12;
  case LedMode::startConfigUpdate:
    return 11;
  case LedMode::startReboot:
    return 10;
  case LedMode::loggerBusyBackgroundProcess:
    return 9;
  case LedMode::updateBootComplete:
    return 8;
  case LedMode::batteryLow:
    return 7;

  // errors
  case LedMode::batterySuperlow:
    return 6;
  case LedMode::noConnectionToDeckbox:
    return 5;
  case LedMode::skipSensorError:
    return 4;
  case LedMode::ntpUpdateFailed:
    return 2;
  case LedMode::configRejected:
    return 1;
  }

  return 0;
}

void ledInit();
void ledControl(LedMode mode);

void askForConfig();
void goDeepSleep();
void fatalError();

void normalUsage();
void interaction();
void errors();

String printBits(uint32_t);

void resetLedBitMask();

#endif
