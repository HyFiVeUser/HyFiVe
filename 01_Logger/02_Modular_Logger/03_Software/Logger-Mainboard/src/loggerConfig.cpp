/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Logger configuration management
 */

#include <ArduinoJson.h>

#include "DebuggingSDLog.h"
#include "DeepSleep.h"
#include "Led.h"
#include "SDCard.h"
#include "SystemVariables.h"
#include "Utility.h"
#include "loggerConfig.h"
#include "loggerConfigValidation.h"

LoggerConfig config;

DynamicJsonDocument doc(40000);

/**
 * @brief Moves a configuration file from source to destination.
 * @param sourcePath Source path of the configuration file.
 * @param destinationPath Destination path for the configuration file.
 * @return bool True if file was moved successfully, false otherwise.
 */
bool moveFileConfig(const char *sourcePath, const char *destinationPath)
{
  File sourceFile = SD.open(sourcePath, FILE_READ);
  if (!sourceFile)
  {
    return false; // Quelldatei konnte nicht geöffnet werden
  }

  File destFile = SD.open(destinationPath, FILE_WRITE);
  if (!destFile)
  {
    sourceFile.close();
    return false; // Target file could not be created
  }

  // Copy the content
  while (sourceFile.available())
  {
    destFile.write(sourceFile.read());
  }

  // Close the files
  sourceFile.close();
  destFile.close();

  // Delete the source file
  return SD.remove(sourcePath);
}

/**
 * @brief Reads and parses a JSON configuration file.
 * @param pfad Path to the JSON file.
 * @return bool True if file was read and parsed successfully, false otherwise.
 */
bool JsonFileRead(const String &pfad)
{
  File file = SD.open(pfad.c_str(), FILE_READ);
  if (!file)
  {
    Serial.println("/loggerConfig/Config.json could not be found on the SD card.");
    return false;
  }

  // Determine the file size
  size_t fileSize = file.size();
  if (fileSize == 0)
  {
    Serial.println("/loggerConfig/Config.json is empty!");
    file.close();
    return false;
  }

  DeserializationError error = deserializeJson(doc, file);
  if (error)
  {
    Serial.println(F("JSON deserialisation failed!"));
    file.close();
    return false;
  }
  file.close();
  return true;
}

/**
 * @brief Configures sensors from JSON data.
 */
void configureSensorsFromJson()
{
  JsonArray sensorsArray = doc["sensors"].as<JsonArray>();
  SensorArraySize = sensorsArray.size();

  if (SensorArraySize > MAX_SENSOR_CREDENTIALS)
    SensorArraySize = MAX_SENSOR_CREDENTIALS; // Limit the size of the arrays

  // Fill the array with the sensor data from the JSON document
  int i = 0;
  for (JsonVariant sensor : sensorsArray)
  {
    configRTC.sensor[i].sensor_id = sensor["sensor_id"];
    configRTC.sensor[i].sample_periode_multiplier = sensor["sample_periode_multiplier"];
    configRTC.sensor[i].sample_cast_periode_multiplier = sensor["sample_cast_periode_multiplier"];
    configRTC.sensor[i].bus_address = sensor["bus_address"];

    // config.sensor[i].calib_coeff_0 = sensor["calib_coeff"]["0"];
    config.sensor[i].calib_coeff_1 = sensor["calib_coeff"]["1"];
    config.sensor[i].calib_coeff_2 = sensor["calib_coeff"]["2"];
    config.sensor[i].calib_coeff_3 = sensor["calib_coeff"]["3"];
    config.sensor[i].calib_coeff_4 = sensor["calib_coeff"]["4"];
    config.sensor[i].calib_coeff_5 = sensor["calib_coeff"]["5"];
    config.sensor[i].calib_coeff_6 = sensor["calib_coeff"]["6"];
    config.sensor[i].calib_coeff_7 = sensor["calib_coeff"]["7"];
    config.sensor[i].calib_coeff_8 = sensor["calib_coeff"]["8"];
    config.sensor[i].calib_coeff_9 = sensor["calib_coeff"]["9"];
    config.sensor[i].calib_coeff_10 = sensor["calib_coeff"]["10"];

    strlcpy(config.sensor[i].serial_number, sensor["serial_number"], sizeof(config.sensor[i].serial_number));

    JsonObject sensorType = sensor["sensor_type"];
    config.sensor[i].sensor_type_id = sensorType["sensor_type_id"];
    strlcpy(configRTC.sensor[i].parameter, sensorType["parameter"], sizeof(configRTC.sensor[i].parameter));
    strlcpy(config.sensor[i].long_name, sensorType["long_name"], sizeof(config.sensor[i].long_name));
    strlcpy(config.sensor[i].unit, sensorType["unit"], sizeof(config.sensor[i].unit));
    strlcpy(config.sensor[i].manufacturer, sensorType["manufacturer"], sizeof(config.sensor[i].manufacturer));
    strlcpy(config.sensor[i].model, sensorType["model"], sizeof(config.sensor[i].model));
    configRTC.sensor[i].parameter_no = sensorType["parameter_no"];
    config.sensor[i].accuracy = sensorType["accuracy"];
    config.sensor[i].resolution = sensorType["resolution"];

    if (++i >= SensorArraySize)
      break; // Prevent more data from being written than the array can hold
  }
}

/**
 * @brief Configures WiFi connections from JSON data.
 */
void configureWifiFromJson()
{
  JsonArray wifiArray = doc["wifi"].as<JsonArray>();
  WifiArraySize = wifiArray.size();

  if (WifiArraySize > MAX_WIFI_CREDENTIALS)
    WifiArraySize = MAX_WIFI_CREDENTIALS; // Limit the size of the arrays

  // Fill the array with the WLAN data from the JSON document
  int i = 0;
  for (JsonVariant wifi : wifiArray)
  {
    if (wifi.is<JsonObject>())
    {
      JsonObject wifiObject = wifi.as<JsonObject>();
      strlcpy(configRTC.wificonfig[i].ssid, wifi["ssid"], sizeof(configRTC.wificonfig[i].ssid));
      strlcpy(configRTC.wificonfig[i].pw, wifi["pw"], sizeof(configRTC.wificonfig[i].pw));
    }
    if (++i >= WifiArraySize)
      break; // Prevent more data from being written than the array can hold
  }
}

/**
 * @brief Configures basic settings from JSON data.
 */
void configureBasicSettingsFromJson()
{
  configRTC.logger_id = doc["logger_id"];

  if (!doc["operation_mode"].isNull())
  {
    strlcpy(config.operation_mode, doc["operation_mode"], sizeof(config.operation_mode));
  }
  else
  {
    config.operation_mode[0] = '\0';
  }

  if (!doc["fw_version"].isNull())
  {
    strlcpy(config.fw_version, doc["fw_version"], sizeof(config.fw_version));
  }
  else
  {
    config.fw_version[0] = '\0';
  }

  configRTC.num_sensors = doc["num_sensors"];
  configRTC.config_update_periode = doc["config_update_periode"];
  configRTC.status_upload_periode = doc["status_upload_periode"];
  configRTC.sample_periode = doc["sample_periode"];
  configRTC.sample_cast_enable = doc["sample_cast_enable"];
  configRTC.sample_cast_periode = doc["sample_cast_periode"];
  configRTC.cast_det_sensor = doc["cast_det_sensor"];
  configRTC.cast_det_sensor_threshold = doc["cast_det_sensor_threshold"];
  configRTC.wet_det_sensor = doc["wet_det_sensor"];
  configRTC.wet_det_periode = doc["wet_det_periode"];
  configRTC.wet_det_threshold = doc["wet_det_threshold"];
  configRTC.dry_det_sensor = doc["dry_det_sensor"];
  configRTC.dry_det_threshold = doc["dry_det_threshold"];
  configRTC.dry_det_verify_delay = doc["dry_det_verify_delay"];
  configRTC.data_upload_retry_periode = doc["data_upload_retry_periode"];
  config.deckunit_id = doc["deckunit_id"];
  config.platform_id = doc["platform_id"];
  config.vessel_id = doc["vessel_id"];
  config.deployment_contact_id = doc["deployment_contact_id"];

  if (!doc["contact_first_name"].isNull())
  {
    strlcpy(config.contact_first_name, doc["contact_first_name"], sizeof(config.contact_first_name));
  }
  else
  {
    config.contact_first_name[0] = '\0';
  }

  if (!doc["contact_last_name"].isNull())
  {
    strlcpy(config.contact_last_name, doc["contact_last_name"], sizeof(config.contact_last_name));
  }
  else
  {
    config.contact_last_name[0] = '\0';
  }

  if (!doc["vessel_name"].isNull())
  {
    strlcpy(config.vessel_name, doc["vessel_name"], sizeof(config.vessel_name));
  }
  else
  {
    config.vessel_name[0] = '\0';
  }

  saveSamplePeriodeToResetAfterUnderwaterMeasurementsEnd = configRTC.sample_periode;
}

/**
 * @brief Compares RTC configuration with JSON configuration file.
 * @return bool True if configurations match, false otherwise.
 */
bool compareRtcWithJsonConfig()
{

  bool compareRtcError = false;

  // Read the latest configuration file from the /loggerConfig folder
  String latestConfigFile = findLatestConfigurationFile("/loggerConfig");
  if (latestConfigFile.isEmpty())
  {
    Log(LogCategoryGeneral, LogLevelERROR, "Error: No configuration file found.");
    compareRtcError = true;
    generalAlarmLed();
  }

  // Open the latest configuration file
  File configFile = SD.open("/loggerConfig/" + latestConfigFile);
  if (!configFile)
  {
    Log(LogCategoryGeneral, LogLevelERROR, "Error when opening the configuration file: ", String(latestConfigFile));
    compareRtcError = true;
    generalAlarmLed();
  }

  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error)
  {
    Log(LogCategoryConfiguration, LogLevelERROR, "Error parsing the configuration file: ", String(latestConfigFile));
    compareRtcError = true;
    generalAlarmLed();
  }

  if (configRTC.logger_id != doc["logger_id"])
  {
    Log(LogCategoryConfiguration, LogLevelERROR, "logger_id does not match.");
    compareRtcError = true;
  }

  if (configRTC.num_sensors != doc["num_sensors"])
  {
    Log(LogCategoryConfiguration, LogLevelERROR, "num_sensors does not match.");
    compareRtcError = true;
  }

  if (configRTC.config_update_periode != doc["config_update_periode"])
  {
    Log(LogCategoryConfiguration, LogLevelERROR, "config_update_periode does not match.");
    compareRtcError = true;
  }

  if (configRTC.status_upload_periode != doc["status_upload_periode"])
  {
    Log(LogCategoryConfiguration, LogLevelERROR, "status_upload_periode does not match.");
    compareRtcError = true;
  }

  if (configRTC.sample_periode != doc["sample_periode"])
  {
    Log(LogCategoryConfiguration, LogLevelERROR, "sample_periode does not match.");
    compareRtcError = true;
  }

  if (configRTC.sample_cast_enable != doc["sample_cast_enable"])
  {
    Log(LogCategoryConfiguration, LogLevelERROR, "sample_cast_enable does not match.");
    compareRtcError = true;
  }

  if (configRTC.sample_cast_periode != doc["sample_cast_periode"])
  {
    Log(LogCategoryConfiguration, LogLevelERROR, "sample_cast_periode does not match.");
    compareRtcError = true;
  }

  if (configRTC.cast_det_sensor != doc["cast_det_sensor"])
  {
    Log(LogCategoryConfiguration, LogLevelERROR, "cast_det_sensor does not match.");
    compareRtcError = true;
  }

  if (configRTC.cast_det_sensor_threshold != doc["cast_det_sensor_threshold"])
  {
    Log(LogCategoryConfiguration, LogLevelERROR, "cast_det_sensor_threshold does not match.");
    compareRtcError = true;
  }

  if (!compareRtcError)
  {
    // Serial.println("The RTC values match the config.json file.");
    bootCounter = 0;
  }
  else
  {
    Log(LogCategoryConfiguration, LogLevelERROR, "The RTC values do not match the config.json file.");
    generalAlarmLed();
  }

  return compareRtcError;
}

/**
 * @brief Validates and loads the configuration.
 */
void validateAndLoadConfig()
{
  findLatestConfigFileUpdateConfig = findLatestConfigurationFile("/loggerConfig");
  if (JsonFileReadValidation("/loggerConfig/" + findLatestConfigFileUpdateConfig))
  {
    validateBasicConfiguration();
    validateWifiConfiguration();
    validateSensorsConfiguration();
    Log(LogCategoryConfiguration, LogLevelDEBUG, "validateSensorsConfiguration");
  }

  if (configUpdateError)
  {
    generalAlarmLed();
  }

  JsonFileRead("/loggerConfig/" + findLatestConfigurationFile("/loggerConfig"));
  configureSensorsFromJson();
  configureWifiFromJson();
  configureBasicSettingsFromJson();
  Log(LogCategoryConfiguration, LogLevelDEBUG, "ConfigFile loaded");
}