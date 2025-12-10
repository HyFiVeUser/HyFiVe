/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Debugging and SD card logging functionality
 */

#include "DebuggingSDLog.h"
// Definiere das Mapping von Log-Kategorien zu Log-Levels
std::map<LogCategory, LogLevel> logSettings = {

    //! Log-Levels
    //*  LogLevelDEBUG
    //*  LogLevelINFO,
    //*  LogLevelWARNING,
    //*  LogLevelERROR

    {LogCategoryGeneral, LogLevelINFO},
    {LogCategorySensors, LogLevelINFO},
    {LogCategoryBMS, LogLevelINFO},
    {LogCategoryCharger, LogLevelINFO},
    {LogCategoryWiFi, LogLevelINFO},
    {LogCategoryMQTT, LogLevelINFO},
    {LogCategorySDCard, LogLevelINFO},
    {LogCategoryRTC, LogLevelINFO},
    {LogCategoryPowerManagement, LogLevelINFO},
    {LogCategoryConfiguration, LogLevelINFO},
    {LogCategoryError, LogLevelINFO},
    {LogCategoryDebug, LogLevelINFO},
    {LogCategoryMeasurement, LogLevelINFO}

};