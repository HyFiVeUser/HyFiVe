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
  ntpUpdateFailed,
  configRejected,
};

void ledInit();
void ledControl(LedMode mode);

void askForConfig();
void goDeepSleep();
void generalError();

void normalUsage();
void interaction();
void errors();

#endif