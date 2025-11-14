/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Battery Management System (BMS) functionality
 */

#ifndef BMS_H
#define BMS_H

#include <Wire.h>

extern TwoWire i2c;

void initBmsAndRtc();
void saveBatteryErrorLog();
void bmsReset();
void manageBatteryCharging();

float getRemainingBatteryPercentage();

uint16_t getTotalBatteryCellVoltage();
uint16_t getRemainingBatteryCapacity();

bool getCellCurrent();

#endif