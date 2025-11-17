#pragma once

#include <Arduino.h>
// Declaration only – no implementation!
void logBmsSnapshot();
bool bmsReset();
void manageBatteryCharging();
bool isPG();
void ensureChargerPinsInit();