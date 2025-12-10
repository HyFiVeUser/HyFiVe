/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Validation of logger configuration
 */

#ifndef LOGGERCONFIGVALIDATION_H
#define LOGGERCONFIGVALIDATION_H

extern int logger_idValidation;
extern bool configUpdateError;

bool JsonFileReadValidation(const String &pfad);

void validateSensorsConfiguration();
void validateWifiConfiguration();
void validateBasicConfiguration();

#endif