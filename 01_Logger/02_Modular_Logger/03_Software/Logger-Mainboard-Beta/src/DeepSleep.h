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

#ifndef DEEPSLEEP_H
#define DEEPSLEEP_H

#include <Arduino.h>

void espDeepSleepSec(uint32_t sleepTimeSec);
void enableExternalWakeup(uint8_t);
void disableWakeupPin(uint8_t);
void inactiveMeasurement();
void getInactiveMeasurement();

#endif
