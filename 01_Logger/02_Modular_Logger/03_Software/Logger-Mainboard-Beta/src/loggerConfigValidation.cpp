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

#include <ArduinoJson.h>

#include "DebuggingSDLog.h"
#include "MQTTManager.h"
#include "SDCard.h"
#include "SystemVariables.h"
#include "Utility.h"
#include "loggerConfigValidation.h"

DynamicJsonDocument docValidation(40000);
int logger_idValidation;
bool configUpdateError = false;

/**
 * @brief Reads and validates a JSON configuration file.
 * @param pfad Path to the JSON file.
 * @return bool True if file was read and validated successfully, false otherwise.
 */
bool JsonFileReadValidation(const String &pfad)
{
  File file = SD.open(pfad.c_str(), FILE_READ);
  if (!file)
  {
    Serial.println(F("/loggerConfig/Config.json could not be found on the SD card."));
    return false;
  }

  // Determine the file size
  size_t fileSize = file.size();
  if (fileSize == 0)
  {
    Serial.println(F("/loggerConfig/Config.json is empty!"));
    file.close();
    return false;
  }

  DeserializationError error = deserializeJson(docValidation, file);
  if (error)
  {
    Serial.println(F("Validation JSON deserialization failed!"));
    file.close();
    return false;
  }
  file.close();
  return true;
}

/**
 * @brief Checks whether a value in the JSON object is a valid ASCII string.
 * @param obj The JSON object.
 * @param key The key of the value to be checked.
 * @param maxLaenge The maximum permitted length of the string.
 * @param sensorArray Sensor arrays (-1 = no arrays).
 */
void isValidAsciiString(JsonVariant obj, const char *key, size_t maxLaenge, int sensorArray = -1)
{
  // Check whether the value is zero
  if (obj[key].isNull())
  {
    return;
  }

  if (!obj.containsKey(key))
  {
    String error = "Key does not exist";

    if (sensorArray != -1)
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + "sensor array: " + sensorArray + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", "sensor array: ", String(sensorArray), " | ", error);
      configUpdateError = true;
    }
    else
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", error);
      configUpdateError = true;
    }
    return;
  }

  // Check whether the value is a character string
  if (!obj[key].is<const char *>())
  {
    String error = "is not a string";

    if (sensorArray != -1)
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + "sensor array: " + sensorArray + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", "sensor array: ", String(sensorArray), " | ", error);
      configUpdateError = true;
    }
    else
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", error);
      configUpdateError = true;
    }
    return;
  }

  const char *wert = obj[key];
  size_t laenge    = strlen(wert);

  if (laenge > maxLaenge)
  {
    String error = "exceeds the maximum length";

    if (sensorArray != -1)
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + "sensor array: " + sensorArray + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", "sensor array: ", String(sensorArray), " | ", error);
      configUpdateError = true;
    }
    else
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", error);
      configUpdateError = true;
    }
    return;
  }

  for (size_t i = 0; i < laenge; ++i)
  {
    if (wert[i] < 0 || wert[i] > 127)
    {
      String error = "contains non-ASCII characters";

      if (sensorArray != -1)
      {
        transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + "sensor array: " + sensorArray + " | " + error).c_str(), "hyfive/ConfigError");
        Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", "sensor array: ", String(sensorArray), " | ", error);
        configUpdateError = true;
      }
      else
      {
        transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + error).c_str(), "hyfive/ConfigError");
        Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", error);
        configUpdateError = true;
      }
      return;
    }
  }
}

/**
 * @brief Checks whether a value in the JSON object is a valid number within a certain range.
 * @param obj The JSON object.
 * @param key The key of the value to be checked.
 * @param min The minimum value.
 * @param max The maximum value.
 * @param sensorArray Sensor arrays (-1 = no arrays).
 */
void validateNumericValue(JsonVariant obj, const char *key, int min, int max, int sensorArray = -1)
{
  // Check whether the value is zero
  if (obj[key].isNull())
  {
    return;
  }

  if (!obj.containsKey(key))
  {
    String error = "Key does not exist";

    if (sensorArray != -1)
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + "sensor array: " + sensorArray + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", "sensor array: ", String(sensorArray), " | ", error);
      configUpdateError = true;
    }
    else
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", error);
      configUpdateError = true;
    }
    return;
  }

  // Check whether the value is a number
  if (!obj[key].is<uint32_t>())
  {
    String error = "is not a number";

    if (sensorArray != -1)
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + "sensor array: " + sensorArray + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", "sensor array: ", String(sensorArray), " | ", error);
      configUpdateError = true;
    }
    else
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", error);
      configUpdateError = true;
    }
    return;
  }

  uint32_t wert = obj[key].as<uint32_t>();

  if (wert < min || wert > max)
  {
    String error = "Value is outside the valid range";

    if (sensorArray != -1)
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + "sensor array: " + sensorArray + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", "sensor array: ", String(sensorArray), " | ", error);
      configUpdateError = true;
    }
    else
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", error);
      configUpdateError = true;
    }
    return;
  }
}

/**
 * @brief Checks whether a value in the JSON object is a valid floating point number.
 * @param sensorType The JSON object.
 * @param key The key of the value to be checked.
 * @param sensorArray Sensor arrays (-1 = no arrays).
 */
void isStringFloatingPoint(const JsonVariant &sensorType, const char *key, int sensorArray = -1)
{
  // Check whether the value is zero
  if (sensorType[key].isNull())
  {
    return;
  }

  // Check whether the parameter exists
  if (!sensorType.containsKey(key))
  {
    String error = "Key does not exist";

    if (sensorArray != -1)
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + "sensor array: " + sensorArray + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", "sensor array: ", String(sensorArray), " | ", error);
      configUpdateError = true;
    }
    else
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", error);
      configUpdateError = true;
    }
    return;
  }

  // Check whether the value is a character string
  if (sensorType[key].is<const char *>())
  {
    String error = "is not a fload";

    if (sensorArray != -1)
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + "sensor array: " + sensorArray + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", "sensor array: ", String(sensorArray), " | ", error);
      configUpdateError = true;
    }
    else
    {
      transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + error).c_str(), "hyfive/ConfigError");
      Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", error);
      configUpdateError = true;
    }
    return;
  }
}

/**
 * @brief Checks the calibration coefficients of a sensor.
 * @param sensorObj The JSON object.
 * @param sensorArray Sensor arrays (-1 = no arrays).
 */
void checkCalibrationCoefficients(JsonObject sensorObj, int sensorArray = -1)
{
  // Path to the `calib_coeff` object in the sensor object
  JsonObject calibCoeffs = sensorObj["calib_coeff"].as<JsonObject>();

  // Number of coefficients to be checked
  const int coeffAnzahl = 10; // Change this according to the actual number

  for (int i = 0; i < coeffAnzahl; ++i)
  {
    // Generate the key dynamically
    String key = String(i);

    // Check whether the current calib_coeff exists and is a number
    if (calibCoeffs.containsKey(key))
    {
      if (!calibCoeffs[key].is<float>())
      {
        String error = "No valid number";

        if (sensorArray != -1)
        {
          transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: calib_coeff " + key + " | " + "sensor array: " + sensorArray + " | " + error).c_str(), "hyfive/ConfigError");
          Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: calib_coeff ", key, " | ", "sensor array: ", String(sensorArray), " | ", error);
          configUpdateError = true;
        }
        else
        {
          transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: calib_coeff " + key + " | " + error).c_str(), "hyfive/ConfigError");
          Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: calib_coeff ", key, " | ", error);
          configUpdateError = true;
        }
        return;
      }
    }
  }
}

/**
 * @brief Checks whether a value in the JSON object is a valid Boolean value.
 * @param var The JSON object.
 * @param key The key of the value to be checked.
 */
void checkBooleanValue(JsonVariant var, const char *key)
{
  // Check whether the value is zero
  if (var[key].isNull())
  {
    return;
  }

  // Check whether the value exists
  if (!var.containsKey(key))
  {
    String error = "Key does not exist";
    transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + error).c_str(), "hyfive/ConfigError");
    Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", error);
    configUpdateError = true;
    return;
  }

  // Check whether the value is a character string
  if (var[key].is<const char *>())
  {
    String error = "is not a bool";
    transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + error).c_str(), "hyfive/ConfigError");
    Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", error);
    configUpdateError = true;
    return;
  }

  // Check whether the value is a number
  if (!var[key].is<uint32_t>())
  {
    String error = "is not a number";
    transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + error).c_str(), "hyfive/ConfigError");
    Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", error);
    configUpdateError = true;
    return;
  }

  // Read the value as a
  uint32_t wert = var[key].as<uint32_t>();

  // Check whether the value is 0 or 1
  if (wert == 0 || wert == 1)
  {
    return;
  }
  else
  {
    String error = "has an invalid value (not 0 or 1)";
    transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "key: " + key + " | " + error).c_str(), "hyfive/ConfigError");
    Log(LogCategoryConfiguration, LogLevelERROR, "Error in: ", findLatestConfigFileUpdateConfig, " - ", "key: ", key, " | ", error);
    configUpdateError = true;
    return;
  }
}

/**
 * @brief Validates the sensors configuration.
 */
void validateSensorsConfiguration()
{
  int i                  = 0;
  JsonArray sensorsArray = docValidation["sensors"].as<JsonArray>();
  for (JsonObject sensorObj : sensorsArray)
  {
    validateNumericValue(sensorObj, "sensor_id", 0, 65535, i);
    validateNumericValue(sensorObj, "sample_periode_multiplier", 0, 255, i);
    validateNumericValue(sensorObj, "sample_cast_periode_multiplier", 0, 255, i);
    validateNumericValue(sensorObj, "bus_address", 0, 255, i);
    checkCalibrationCoefficients(sensorObj, i);
    isValidAsciiString(sensorObj, "serial_number", 45, i);
    JsonObject sensorType = sensorObj["sensor_type"].as<JsonObject>();
    isValidAsciiString(sensorType, "parameter", 45, i);
    isValidAsciiString(sensorType, "long_name", 100, i);
    isValidAsciiString(sensorType, "unit", 45, i);
    isValidAsciiString(sensorType, "manufacturer", 45, i);
    isValidAsciiString(sensorType, "model", 45, i);
    validateNumericValue(sensorType, "parameter_no", 0, 255, i);
    isStringFloatingPoint(sensorType, "accuracy", i);
    isStringFloatingPoint(sensorType, "resolution", i);
    i++;
  }
}

/**
 * @brief Validates the WiFi configuration.
 */
void validateWifiConfiguration()
{
  int i               = 0;
  JsonArray wifiArray = docValidation["wifi"].as<JsonArray>();
  for (JsonObject wifiObj : wifiArray)
  {
    isValidAsciiString(wifiObj, "ssid", 15, i);
    isValidAsciiString(wifiObj, "pw", 16, i);
    i++;
  }
  if (i == 0 && wificonfigRtc)
  {
    transmitUpdateMessage(("LoggerConfigFile | configUpdate has no wifi data available "), "hyfive/ConfigError");
    Log(LogCategoryConfiguration, LogLevelERROR, "no wifi data available");
    configUpdateError = true;
  }
}

/**
 * @brief Validates the basic configuration.
 */
void validateBasicConfiguration()
{
  logger_idValidation = docValidation["logger_id"].as<int>();
  validateNumericValue(docValidation, "logger_id", 0, 65535);
  isValidAsciiString(docValidation, "operation_mode", 45);
  isValidAsciiString(docValidation, "fw_version", 100);
  validateNumericValue(docValidation, "num_sensors", 0, 255);
  validateNumericValue(docValidation, "config_update_periode", 0, 65535);
  validateNumericValue(docValidation, "status_upload_periode", 0, 65535);
  validateNumericValue(docValidation, "sample_periode", 0, 65535);
  checkBooleanValue(docValidation, "sample_cast_enable");
  validateNumericValue(docValidation, "sample_cast_periode", 0, 255);
  validateNumericValue(docValidation, "cast_det_sensor", 0, 65535);
  validateNumericValue(docValidation, "cast_det_sensor_threshold", 0, 65535);
  validateNumericValue(docValidation, "wet_det_sensor", 0, 65535);
  validateNumericValue(docValidation, "wet_det_periode", 0, 65535);
  isStringFloatingPoint(docValidation, "wet_det_threshold");
  validateNumericValue(docValidation, "dry_det_sensor", 0, 65535);
  validateNumericValue(docValidation, "dry_det_threshold", 0, 65535);
  validateNumericValue(docValidation, "dry_det_verify_delay", 0, 65535);
  validateNumericValue(docValidation, "data_upload_retry_periode", 0, 65535);
  validateNumericValue(docValidation, "deckunit_id", 0, 65535);
  validateNumericValue(docValidation, "platform_id", 0, 65535);
  validateNumericValue(docValidation, "vessel_id", 0, 65535);
  isValidAsciiString(docValidation, "vessel_name", 45);
  validateNumericValue(docValidation, "deployment_contact_id", 0, 65535);
  isValidAsciiString(docValidation, "contact_first_name", 45);
  isValidAsciiString(docValidation, "contact_last_name", 45);
  if (docValidation.containsKey("led_Color_Config"))
  {
    validateNumericValue(docValidation, "led_Color_Config", 0, 999);
  }
  if (docValidation.containsKey("inactive_Measurement_periode"))
  {
    validateNumericValue(docValidation, "inactive_Measurement_periode", 0, 86400);
  }
  if (docValidation.containsKey("rgb_version"))
  {
    validateNumericValue(docValidation, "rgb_version", 0, 255);
  }
}
