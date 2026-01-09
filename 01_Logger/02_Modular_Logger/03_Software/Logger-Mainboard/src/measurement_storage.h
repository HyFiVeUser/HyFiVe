#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <RTClib.h> 

static const char* const MEASUREMENTS_ROOT_DIR = "/measurements";

void initMeasurementStorage();
void notifyMeasurementPaused();
void storeSensorMeasurements();
void updateMeasurementIndex(int32_t newDelta, int32_t transferredDelta = 0);
bool getMeasurementIndexCounts(uint32_t &measuredCount, uint32_t &transferredCount);
bool ensureCurrentFile(const DateTime &now);
bool validateMeasurementIndex();