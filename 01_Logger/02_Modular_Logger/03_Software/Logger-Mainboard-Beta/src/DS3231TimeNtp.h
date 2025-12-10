/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: RTC (DS3231) and NTP time synchronization functions
 */

#ifndef DS3231TIMENTP_H
#define DS3231TIMENTP_H

#include <Wire.h>

bool initRTC(TwoWire *wireInstance);
bool synchronizeTimeWithNTP();

unsigned long getCurrentTimeFromRTC();

String formatLocalTimeAsISOString();
String getLocalTimeAsStringBackup();
String getLocalTimeAsStringLog();

#endif
