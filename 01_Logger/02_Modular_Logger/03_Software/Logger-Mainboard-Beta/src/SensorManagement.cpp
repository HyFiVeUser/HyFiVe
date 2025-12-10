/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Sensor management and data collection
 */

#include <ArduinoJson.h>
#include <atomic>
#include <iomanip>

#include "BMS.h"
#include "Charger.h"
#include "DS3231TimeNtp.h"
#include "DebuggingSDLog.h"
#include "DeepSleep.h"
#include "Led.h"
#include "LoggerHER.h"
#include "MQTTManager.h"
#include "SDCard.h"
#include "SensorManagement.h"
#include "SystemVariables.h"
#include "Utility.h"
#include "WifiNetwork.h"
#include "loggerConfig.h"
#include "loggerConfigValidation.h"
#include "sample_cast.h"

int64_t AdapterSensorRawValue[MAX_SENSOR_CREDENTIALS];
uint8_t AdapterSensorTypeID[MAX_SENSOR_CREDENTIALS];
uint8_t AdapterConfig_Value_Type[MAX_SENSOR_CREDENTIALS];
int64_t AdapterSensorCalcValue[MAX_SENSOR_CREDENTIALS];
uint32_t bootCount = 0;
uint8_t MAX_NUM_SENSORS = MAX_SENSOR_CREDENTIALS;

TwoWire i2cBus(0);
SPIClass *spi_interface;
#define SD_CS 18
#define SDA_PIN 4
#define SCL_PIN 5

bool measurementSuccessful[MAX_SENSOR_CREDENTIALS];
float sensorValue[MAX_SENSOR_CREDENTIALS];
float sensorValueRaw[MAX_SENSOR_CREDENTIALS];

unsigned long tStart = 0;
unsigned long tEnd = 0;
float shortestSensorWaitingTime = 0;

/**
 * @brief Initializes the logger.
 */
void initializeLogger()
{
  if (!isFirstBoot)
  {
    delay(5000); // at least 5 seconds are required for initialization when the interface board is started for the first time
  }

  Logger.Init(i2cBus,
              AdapterSensorTypeID,
              AdapterConfig_Value_Type,
              AdapterSensorRawValue,
              AdapterSensorCalcValue,
              bootCount,
              ((int)SD_CS),
              ((int)SDA_PIN),
              ((int)SCL_PIN),
              spi_interface);

  for (int i = 0; i < MAX_NUM_SENSORS; i++)
  {
    AdapterSensorRawValue[i] = 0;
    AdapterSensorCalcValue[i] = 0;
  }

  Logger.begin_I2C();
}

/**
 * @brief Converts a uint32_t to float using IEEE-754 Floating Point conversion.
 * @param decimalValue The uint32_t value to convert.
 * @return float The converted float value.
 */
float floatingPointConvert(uint32_t decimalValue)
{
  union
  {
    uint32_t i;
    float f;
  } converter;

  converter.i = decimalValue;
  return converter.f;
}

void addSensorToErrorSkip(int sensorNumber)
{
  // Check whether the sensor is already in the list
  for (int i = 0; i < errorSkipSensorSize; i++)
  {
    if (errorSkipSensor[i] == sensorNumber)
    {
      return; // Sensor is already in the list, nothing to do
    }
  }

  // Add the sensor to the list if there is still space
  if (errorSkipSensorSize < MAX_SENSOR_CREDENTIALS)
  {
    errorSkipSensor[errorSkipSensorSize++] = sensorNumber;
    Log(LogCategorySensors, LogLevelERROR, "sensor_id ", String(configRTC.sensor[sensorNumber].sensor_id), " added to errorSkipSensor");
  }
  else
  {
    Log(LogCategorySensors, LogLevelERROR, "errorSkipSensor is full, cannot add sensor ", String(sensorNumber));
  }
}

void detectConnecteOxygenSensor(uint8_t sensorNumber)
{
  for (int i = 0; i < errorSkipSensorSize; i++)
  {
    if (temperatureSensorBusAddress == configRTC.sensor[errorSkipSensor[i]].bus_address)
    {
      addSensorToErrorSkip(sensorNumber);
      return;
    }
  }

  if (oxygenSensorBusAddress != 255 && temperatureSensorBusAddress != 255)
  {
    for (int x = 0; x < 100; x++)
    {
      Logger.getInterfaceRDY(temperatureSensorBusAddress);
      uint32_t interfaceRDY = AdapterSensorRawValue[temperatureSensorBusAddress];

      if (interfaceRDY == 1)
      {
        break;
      }
      delay(10);
    }

    Logger.Measure(temperatureSensorBusAddress, temperatureSensorParameter);
    uint32_t rawValue = AdapterSensorRawValue[temperatureSensorBusAddress];
    float sensorValueTemp = floatingPointConvert(rawValue);
    Log(LogCategorySensors, LogLevelDEBUG, "sensorValueTemp: ", String(sensorValueTemp));
    setSensorTempToInterface(oxygenSensorBusAddress, sensorValueTemp);
  }
}

/**
 * @brief Performs a measurement for a specific sensor.
 * @param sensorNumber The number of the sensor to measure.
 */
void performSensorMeasurement(int sensorNumber)
{
  Log(LogCategorySensors, LogLevelDEBUG, "interfaceRdyErrorCounter: ", String(interfaceRdyErrorCounter));

  if (oxygenSensorBusAddress == configRTC.sensor[sensorNumber].bus_address)
  {
    detectConnecteOxygenSensor(sensorNumber);
  }

  uint64_t start_time = esp_timer_get_time() / 1000; // Start time in milliseconds
  bool measuringTimeTooLong = true;
  bool interfaceErrorRdy2 = false;

  while ((esp_timer_get_time() / 1000 - start_time) < 5000)
  {
    Log(LogCategorySensors, LogLevelDEBUG, "esp_timer_get_time: ", String(esp_timer_get_time() / 1000 - start_time));
    Logger.getInterfaceRDY(configRTC.sensor[sensorNumber].bus_address);
    uint32_t interfaceRDY = AdapterSensorRawValue[configRTC.sensor[sensorNumber].bus_address];

    if (interfaceRDY == 1)
    {
      measuringTimeTooLong = false;
      if (interfaceErrorSensorId == 0 || interfaceErrorSensorId == configRTC.sensor[sensorNumber].sensor_id)
      {
        interfaceRdyErrorCounter = 0;
        interfaceErrorSensorId = 0;
      }
      break;
    }

    if (interfaceRDY == 2)
    {
      Log(LogCategorySensors, LogLevelDEBUG, "sensor_id ", String(configRTC.sensor[sensorNumber].sensor_id), " returns no value");
      interfaceError = true;
      interfaceErrorRdy2 = true;
      interfaceErrorSensorId = configRTC.sensor[sensorNumber].sensor_id;

      if (interfaceRdyErrorCounter >= 5)
      {
        interfaceErrorSensorId = 0;
        addSensorToErrorSkip(sensorNumber);
      }

      Logger.interfaceSoftwareReset(configRTC.sensor[sensorNumber].bus_address);
      interfaceRdyErrorCounter++;
      sensorCalibToInterfaceIfRdyErrorCounter = 4;

      break;
    }
  }

  if (measuringTimeTooLong && !interfaceErrorRdy2)
  {
    Log(LogCategorySensors, LogLevelDEBUG, "sensor_id ", String(configRTC.sensor[sensorNumber].sensor_id), " sensor measuring time too long");
    interfaceError = true;
    interfaceErrorSensorId = configRTC.sensor[sensorNumber].sensor_id;

    if (interfaceRdyErrorCounter >= 5)
    {
      interfaceErrorSensorId = 0;
      addSensorToErrorSkip(sensorNumber);
    }

    Logger.interfaceSoftwareReset(configRTC.sensor[sensorNumber].bus_address);
    interfaceRdyErrorCounter++;
    sensorCalibToInterfaceIfRdyErrorCounter = 4;
  }

  if (!measuringTimeTooLong && !interfaceErrorRdy2)
  {
    Logger.Measure(configRTC.sensor[sensorNumber].bus_address, configRTC.sensor[sensorNumber].parameter_no);
    uint32_t rawValue = AdapterSensorRawValue[configRTC.sensor[sensorNumber].bus_address];
    sensorValue[sensorNumber] = floatingPointConvert(rawValue);

    Logger.Measure(configRTC.sensor[sensorNumber].bus_address, (configRTC.sensor[sensorNumber].parameter_no) + 2);
    sensorValueRaw[sensorNumber] = AdapterSensorRawValue[configRTC.sensor[sensorNumber].bus_address];
  }
  else
  {
    sensorValue[sensorNumber] = -1;
    sensorValueRaw[sensorNumber] = -1;
  }
  Log(LogCategorySensors, LogLevelDEBUG, "sensor_id: ", String(configRTC.sensor[sensorNumber].sensor_id), " sensor value: ", String(sensorValue[sensorNumber]), " sensor value raw: ", String(sensorValueRaw[sensorNumber]), " sensor parameter: ", String(configRTC.sensor[sensorNumber].parameter));
  measurementSuccessful[sensorNumber] = true;
}

/**
 * @brief Updates sensor measurements based on elapsed time.
 */
void updateSensorMeasurements()
{
  for (int sensorNumber = 0; sensorNumber < numberOfActiveSensors; ++sensorNumber)
  {
    if ((float)(totalOperationTime - lastSensorMeasurementTime[sensorNumber]) >= intervalSensorArray[sensorNumber])
    {
      bool shouldSkip = false;

      // Check whether the current sensor should be skipped
      for (int i = 0; i < errorSkipSensorSize; i++)
      {
        if (sensorNumber == errorSkipSensor[i])
        {
          shouldSkip = true;
          break;
        }
      }

      if (shouldSkip)
      {

        if (configRTC.sensor[sensorNumber].bus_address == waterDetectionSensorBusAddress || configRTC.sensor[sensorNumber].bus_address == dryDetectionSensorBusAddress || configRTC.sensor[sensorNumber].bus_address == castDetectionSensorBusAddress)
        {
          Log(LogCategorySensors, LogLevelERROR, " I2C|detection Sensor not found, bus address: ", String(configRTC.sensor[sensorNumber].bus_address));
          generalError();
        }

        Log(LogCategorySensors, LogLevelDEBUG, "Skipping sensor: ", String(sensorNumber));
      }
      else
      {
        performSensorMeasurement(sensorNumber);
      }

      lastSensorMeasurementTime[sensorNumber] = totalOperationTime;
    }
  }
}

/**
 * @brief Starts conversion for underwater operations.
 */
void startConversionformUnderWaterOperations()
{
  for (int sensorNumber = 0; sensorNumber < numberOfActiveSensors; ++sensorNumber)
  {
    if ((float)(totalOperationTime - lastSensorMeasurementTime[sensorNumber]) >= intervalSensorArray[sensorNumber])
    {
      bool shouldSkip = false;

      // Check whether the current sensor should be skipped
      for (int i = 0; i < errorSkipSensorSize; i++)
      {
        if (sensorNumber == errorSkipSensor[i])
        {
          shouldSkip = true;
          break;
        }
      }

      if (shouldSkip)
      {
        continue; // Skip this sensor
      }

      Logger.startConversionAll(configRTC.sensor[sensorNumber].bus_address);
      Log(LogCategorySensors, LogLevelDEBUG, "startConversionAll: ", String(configRTC.sensor[sensorNumber].bus_address));
    }
  }
  delay(100);
}

/**
 * @brief Starts conversion for initial measurement.
 */
void startConversionForPerformInitialMeasurement()
{
  for (int sensorNumber = 0; sensorNumber < numberOfActiveSensors; ++sensorNumber)
  {
    bool shouldSkip = false;

    // Check whether the current sensor should be skipped
    for (int i = 0; i < errorSkipSensorSize; i++)
    {
      if (sensorNumber == errorSkipSensor[i])
      {
        shouldSkip = true;
        break;
      }
    }

    if (shouldSkip)
    {
      continue; // Skip this sensor
    }

    Logger.startConversionAll(configRTC.sensor[sensorNumber].bus_address);
    Log(LogCategorySensors, LogLevelDEBUG, "startConversionAll: ", String(configRTC.sensor[sensorNumber].bus_address));
  }
  delay(100);
}

/**
 * @brief Performs initial measurement for all sensors.
 */
void performInitialMeasurement()
{
  enable5V();
  enable12V();
  Logger.sensorWakeupAll();
  startConversionForPerformInitialMeasurement();

  for (int sensorNumber = 0; sensorNumber < numberOfActiveSensors; ++sensorNumber)
  {
    bool shouldSkip = false;

    // Check whether the current sensor should be skipped
    for (int i = 0; i < errorSkipSensorSize; i++)
    {
      if (sensorNumber == errorSkipSensor[i])
      {
        shouldSkip = true;
        break;
      }
    }

    if (shouldSkip)
    {

      if (configRTC.sensor[sensorNumber].bus_address == waterDetectionSensorBusAddress || configRTC.sensor[sensorNumber].bus_address == dryDetectionSensorBusAddress || configRTC.sensor[sensorNumber].bus_address == castDetectionSensorBusAddress)
      {
        Log(LogCategorySensors, LogLevelERROR, " I2C|detection Sensor not found, bus address: ", String(configRTC.sensor[sensorNumber].bus_address));
        generalError();
      }

      Log(LogCategorySensors, LogLevelDEBUG, "Skipping sensor: ", String(sensorNumber));
    }
    else
    {
      performSensorMeasurement(sensorNumber);
    }

    lastSensorMeasurementTime[sensorNumber] = totalOperationTime;
  }

  // if (!interfaceError){interfaceRdyErrorCounter = 0;}

  disable5V();
  disable12V();
  interfaceSleep();
}

/**
 * @brief Calculates the shortest waiting time for the next sensor measurement.
 * @return float The shortest waiting time in seconds.
 */
float calculateShortestSensorWaitTime()
{
  float shortestWaitingTime = ULONG_MAX;
  for (int i = 0; i < numberOfActiveSensors; i++)
  {
    float timeToNextMeasurement = intervalSensorArray[i] - (totalOperationTime - lastSensorMeasurementTime[i]);
    if (timeToNextMeasurement < shortestWaitingTime)
    {
      shortestWaitingTime = timeToNextMeasurement;
    }
  }
  return shortestWaitingTime;
}

/**
 * @brief Writes measurement data to a file.
 */
void writeMeasurementDataToFile()
{
  bool valuePresent = false;

  StaticJsonDocument<1024> doc;

  doc["time"] = formatLocalTimeAsISOString();
  doc["logger_id"] = configRTC.logger_id;
  doc["deployment_id"] = deployment_id;

  // Iterate over all sensors and add their data if the measurement was successful
  for (int i = 0; i < numberOfActiveSensors; i++)
  {
    if (measurementSuccessful[i])
    {
      doc[configRTC.sensor[i].parameter] = String(sensorValue[i]);
      doc[String(configRTC.sensor[i].parameter) + "_raw"] = String(sensorValueRaw[i]);
      valuePresent = true;
    }
  }

  if (valuePresent)
  {
    ledSensorTrue = true;

    String daten = "";
    serializeJson(doc, daten);

    appendDataToFile("/measurements/measurement.json", daten);

    // sampleCast
    for (int i = 0; i < numberOfActiveSensors; i++)
    {
      if (configRTC.sensor[i].sensor_id == configRTC.cast_det_sensor)
      {
        if (!sensorValue[i] == 0)
        {
          File datei = SD.open("/measurements/sample_cast.txt", FILE_APPEND);
          if (datei)
          {
            datei.print(getCurrentTimeFromRTC());
            datei.print(",");
            datei.println(sensorValue[i]);
          }
          datei.close();
        }
      }
    }
  }
}

/**
 * @brief Detects connected sensor devices.
 */
void detectConnectedSensorDevices()
{
  uint8_t SensorArrayCount = 0;
  uint8_t setSensorCalibCount = 0;
  int64_t sensorTypeId = 0;
  bool setSensorTypeId = false;

  for (int i = 0; i < SensorArraySize; i++)
  {
    if (AdapterSensorTypeID[configRTC.sensor[i].bus_address] != 0)
    {
      ++SensorArrayCount;

      Serial.print("InterfaceVersion:       ");
      interfaceVersion(configRTC.sensor[i].bus_address);
      Serial.print("interfaceSensorVoltage: ");
      interfaceSensorVoltage(configRTC.sensor[i].bus_address);
      Serial.print("interfaceParameter:     ");
      interfaceParameterName(configRTC.sensor[i].bus_address);

      Logger.getFwVersion(configRTC.sensor[i].bus_address);
      uint8_t FwVersion = AdapterSensorRawValue[configRTC.sensor[i].bus_address];
      Log(LogCategorySensors, LogLevelINFO, "Interfaceboard: FWVersion: ", String(FwVersion), " | ", "sensor_id: ", String(configRTC.sensor[i].sensor_id), " | ", "model: ", String(config.sensor[i].model), " | ", "long_name: ", String(config.sensor[i].long_name), " | ", "sensor_type_id: ", String(config.sensor[i].sensor_type_id), " | ", "bus_address: ", String(configRTC.sensor[i].bus_address));

      for (int id = 0; id < 4; id++)
      {
        Logger.getInterfaceVersion(configRTC.sensor[i].bus_address, id);
        sensorTypeId = AdapterSensorRawValue[configRTC.sensor[i].bus_address];
        Serial.println("sensorTypeId");
        Serial.println(sensorTypeId);
        if (sensorTypeId == config.sensor[i].sensor_type_id)
        {
          setSensorTypeId = true;
          Serial.println("true");
        }
      }

      if (!isFirstBoot)
      {
        setSensorCalibCount = 0;

        if (!setSensorTypeId)
        {
          Log(LogCategoryConfiguration, LogLevelERROR, "sensor_type_id: ", String(sensorTypeId), " != ", String(config.sensor[i].sensor_type_id), " do not match");
          transmitUpdateMessage(("sensor_type_id: " + String(sensorTypeId) + " != " + String(config.sensor[i].sensor_type_id) + " do not match").c_str(), "hyfive/ConfigError");
          hasSensorError = true;
        }
        else
        {
          if (config.sensor[i].calib_coeff_1 != 0.00)
            setSensorCalibToInterface(configRTC.sensor[i].bus_address, 1, config.sensor[i].calib_coeff_1);
          else
          {
            setSensorCalibCount++;
          }

          if (config.sensor[i].calib_coeff_2 != 0.00)
            setSensorCalibToInterface(configRTC.sensor[i].bus_address, 2, config.sensor[i].calib_coeff_2);
          else
          {
            setSensorCalibCount++;
          }

          if (config.sensor[i].calib_coeff_3 != 0.00)
            setSensorCalibToInterface(configRTC.sensor[i].bus_address, 3, config.sensor[i].calib_coeff_3);
          else
          {
            setSensorCalibCount++;
          }

          if (config.sensor[i].calib_coeff_4 != 0.00)
            setSensorCalibToInterface(configRTC.sensor[i].bus_address, 4, config.sensor[i].calib_coeff_4);
          else
          {
            setSensorCalibCount++;
          }

          if (config.sensor[i].calib_coeff_5 != 0.00)
            setSensorCalibToInterface(configRTC.sensor[i].bus_address, 5, config.sensor[i].calib_coeff_5);
          else
          {
            setSensorCalibCount++;
          }

          if (config.sensor[i].calib_coeff_6 != 0.00)
            setSensorCalibToInterface(configRTC.sensor[i].bus_address, 6, config.sensor[i].calib_coeff_6);
          else
          {
            setSensorCalibCount++;
          }

          if (config.sensor[i].calib_coeff_7 != 0.00)
            setSensorCalibToInterface(configRTC.sensor[i].bus_address, 7, config.sensor[i].calib_coeff_7);
          else
          {
            setSensorCalibCount++;
          }

          if (config.sensor[i].calib_coeff_8 != 0.00)
            setSensorCalibToInterface(configRTC.sensor[i].bus_address, 8, config.sensor[i].calib_coeff_8);
          else
          {
            setSensorCalibCount++;
          }

          if (config.sensor[i].calib_coeff_9 != 0.00)
            setSensorCalibToInterface(configRTC.sensor[i].bus_address, 9, config.sensor[i].calib_coeff_9);
          else
          {
            setSensorCalibCount++;
          }

          if (config.sensor[i].calib_coeff_10 != 0.00)
            setSensorCalibToInterface(configRTC.sensor[i].bus_address, 10, config.sensor[i].calib_coeff_10);
          else
          {
            setSensorCalibCount++;
          }
        }

        Logger.getSensorWakeupTime(configRTC.sensor[i].bus_address);
        uint16_t sensorWakeupTime = AdapterSensorRawValue[configRTC.sensor[i].bus_address];
        if (sensorWakeupTime > longestSensorWakeupTime)
        {
          longestSensorWakeupTime = sensorWakeupTime;
        }

        Logger.getCalibrated(configRTC.sensor[i].bus_address);
        bool interfaceCalibrated = AdapterSensorRawValue[configRTC.sensor[i].bus_address];

        if (!interfaceCalibrated)
        {
          transmitUpdateMessage(("sensor_type_id: " + String(config.sensor[i].sensor_type_id) + ", interfaceboard not calibrated").c_str(), "hyfive/ConfigError");
          Log(LogCategorySensors, LogLevelERROR, "sensor_type_id: ", String(config.sensor[i].sensor_type_id), ", interfaceboard not calibrated: ");
        }
      }

      // find a temperature bus_address
      if ((interfaceParameterName(configRTC.sensor[i].bus_address)) == "Temperature")
      {
        temperatureSensorBusAddress = configRTC.sensor[i].bus_address;
        temperatureSensorParameter = configRTC.sensor[i].parameter_no;
      }

      // find a PyroPicoO2_oxycap_sub_oxycap_hs_sub bus_address
      if ((interfaceVersion(configRTC.sensor[i].bus_address)) == "PyroPicoO2_oxycap_sub__oxycap_hs_sub")
      {
        oxygenSensorBusAddress = configRTC.sensor[i].bus_address;
      }
    }
    else
    {
      Log(LogCategorySensors, LogLevelERROR, "LoggerConfigFile | sensor not found | sensor_id: ", String(configRTC.sensor[i].sensor_id), " bus_address: ", String(configRTC.sensor[i].bus_address), " parameter: ", String(configRTC.sensor[i].parameter));
      transmitUpdateMessage(("LoggerConfigFile | sensor not found | sensor_id: " + String(configRTC.sensor[i].sensor_id) + " bus_address: " + String(configRTC.sensor[i].bus_address) + " parameter: " + String(configRTC.sensor[i].parameter)).c_str(), "hyfive/ConfigError");
      transmitUpdateMessage(("LoggerConfigFile | sensor not found | bus_address: " + String(configRTC.sensor[i].bus_address) + " nicht erreichbar").c_str(), "hyfive/ConfigError");

      hasSensorError = true;
    }
  }
  if (SensorArrayCount != configRTC.num_sensors || SensorArrayCount != SensorArraySize)
  {
    Log(LogCategorySensors, LogLevelDEBUG, "LoggerConfigFile | Incorrect number of sensors. Expected: ", String(SensorArraySize), " Actual: ", String(SensorArrayCount));

    if (currentIncorrectNumberOfsensors == 3)
    {
      Log(LogCategorySensors, LogLevelERROR, "LoggerConfigFile | Incorrect number of sensors");
    }
    else
    {
      Log(LogCategorySensors, LogLevelERROR, "LoggerConfigFile | Incorrect number of sensors Counter: ", String(currentIncorrectNumberOfsensors));
      currentIncorrectNumberOfsensors++;
      espDeepSleepSec(1);
    }

    hasSensorError = true;
  }

  if (hasSensorError)
  {
    String foundAddresses = "";

    for (int bus_address_suche = 0; bus_address_suche < sizeof(AdapterSensorTypeID); bus_address_suche++)
    {
      if (AdapterSensorTypeID[bus_address_suche] != 0)
      {
        foundAddresses += String(bus_address_suche) + ", ";
      }
    }

    if (foundAddresses.length() > 0)
    {
      // Remove the last comma and space
      foundAddresses = foundAddresses.substring(0, foundAddresses.length() - 2);

      Log(LogCategorySensors, LogLevelERROR, "LoggerConfigFile | all bus addresses found: ", foundAddresses);
      transmitUpdateMessage(("LoggerConfigFile | all bus addresses found: " + foundAddresses).c_str(), "hyfive/ConfigError");
    }
    else
    {
      Log(LogCategorySensors, LogLevelERROR, "LoggerConfigFile | sensor not found |no bus addresses found");
      transmitUpdateMessage("LoggerConfigFile | sensor not found | no bus addresses found", "hyfive/ConfigError");
      hasSensorError = true;
      isLoggerBusy = false;
      generalError();
    }
  }
}

/**
 * @brief Sets calibration data for a sensor interface.
 * @param bus_address The I2C bus address of the sensor.
 * @param index The index of the calibration data.
 * @param calib The calibration value to set.
 */
void sensorCalibToInterface()
{
  for (int i = 0; i < SensorArraySize; i++)
  {
    if (AdapterSensorTypeID[configRTC.sensor[i].bus_address] != 0)
    {
      if (config.sensor[i].calib_coeff_1 != 0.00)
        setSensorCalibToInterface(configRTC.sensor[i].bus_address, 1, config.sensor[i].calib_coeff_1);

      if (config.sensor[i].calib_coeff_2 != 0.00)
        setSensorCalibToInterface(configRTC.sensor[i].bus_address, 2, config.sensor[i].calib_coeff_2);

      if (config.sensor[i].calib_coeff_3 != 0.00)
        setSensorCalibToInterface(configRTC.sensor[i].bus_address, 3, config.sensor[i].calib_coeff_3);

      if (config.sensor[i].calib_coeff_4 != 0.00)
        setSensorCalibToInterface(configRTC.sensor[i].bus_address, 4, config.sensor[i].calib_coeff_4);

      if (config.sensor[i].calib_coeff_5 != 0.00)
        setSensorCalibToInterface(configRTC.sensor[i].bus_address, 5, config.sensor[i].calib_coeff_5);

      if (config.sensor[i].calib_coeff_6 != 0.00)
        setSensorCalibToInterface(configRTC.sensor[i].bus_address, 6, config.sensor[i].calib_coeff_6);

      if (config.sensor[i].calib_coeff_7 != 0.00)
        setSensorCalibToInterface(configRTC.sensor[i].bus_address, 7, config.sensor[i].calib_coeff_7);

      if (config.sensor[i].calib_coeff_8 != 0.00)
        setSensorCalibToInterface(configRTC.sensor[i].bus_address, 8, config.sensor[i].calib_coeff_8);

      if (config.sensor[i].calib_coeff_9 != 0.00)
        setSensorCalibToInterface(configRTC.sensor[i].bus_address, 9, config.sensor[i].calib_coeff_9);

      if (config.sensor[i].calib_coeff_10 != 0.00)
        setSensorCalibToInterface(configRTC.sensor[i].bus_address, 10, config.sensor[i].calib_coeff_10);
    }
  }
}

/**
 * @brief Detects dry, wet, and cast sensors.
 */
void detectDryWetCastSensors()
{
  bool sensorFoundWetDetSensor = false;
  bool sensorFoundDryDetSensor = false;
  bool sensorFoundCastDetSensor = false;

  // numberOfActiveSensors = Logger.AdapterNum_Sensors;
  numberOfActiveSensors = SensorArraySize;

  for (int i = 0; i < numberOfActiveSensors; i++)
  {
    if (AdapterSensorTypeID[configRTC.sensor[i].bus_address] != 0)
    {
      if (configRTC.sensor[i].sensor_id == configRTC.wet_det_sensor)
      {
        waterDetectionSensorBusAddress = configRTC.sensor[i].bus_address;
        wetDetSensorParameterNo = configRTC.sensor[i].parameter_no;
        sensorFoundWetDetSensor = true;
        Log(LogCategorySensors, LogLevelDEBUG, "LoggerConfigFile | waterDetectionSensorBusAddress detected | sensor_id: ", String(configRTC.sensor[i].sensor_id), " bus_address: ", String(configRTC.sensor[i].bus_address), " parameter: ", String(configRTC.sensor[i].parameter));
        break;
      }
    }
  }

  if (!sensorFoundWetDetSensor)
  {
    Log(LogCategorySensors, LogLevelERROR, "LoggerConfigFile | waterDetectionSensorBusAddress not found | sensor_id: ", String(configRTC.wet_det_sensor));
    transmitUpdateMessage(("LoggerConfigFile | waterDetectionSensorBusAddress not found | sensor_id: " + String(configRTC.wet_det_sensor)).c_str(), "hyfive/ConfigError");
    hasSensorError = true;
  }

  for (int i = 0; i < numberOfActiveSensors; i++)
  {
    if (AdapterSensorTypeID[configRTC.sensor[i].bus_address] != 0)
    {
      if (configRTC.sensor[i].sensor_id == configRTC.dry_det_sensor)
      {
        dryDetectionSensorBusAddress = configRTC.sensor[i].bus_address;
        dryDetSensorParameterNo = configRTC.sensor[i].parameter_no;
        sensorFoundDryDetSensor = true;
        Log(LogCategorySensors, LogLevelDEBUG, "LoggerConfigFile | dryDetectionSensorBusAddress detected | sensor_id: ", String(configRTC.sensor[i].sensor_id), " bus_address: ", String(configRTC.sensor[i].bus_address), " parameter: ", String(configRTC.sensor[i].parameter));
        break;
      }
    }
  }

  if (!sensorFoundDryDetSensor)
  {
    Log(LogCategorySensors, LogLevelERROR, "LoggerConfigFile | dryDetectionSensorBusAddress not found | sensor_id: ", String(configRTC.dry_det_sensor));
    transmitUpdateMessage(("LoggerConfigFile | dryDetectionSensorBusAddress not found | sensor_id: " + String(configRTC.dry_det_sensor)).c_str(), "hyfive/ConfigError");
    hasSensorError = true;
  }

  for (int i = 0; i < numberOfActiveSensors; i++)
  {
    if (AdapterSensorTypeID[configRTC.sensor[i].bus_address] != 0)
    {
      if (configRTC.sensor[i].sensor_id == configRTC.cast_det_sensor)
      {
        castDetectionSensorBusAddress = configRTC.sensor[i].bus_address;
        castDetSensorParameterNo = configRTC.sensor[i].parameter_no;
        sensorFoundCastDetSensor = true;
        Log(LogCategorySensors, LogLevelDEBUG, "LoggerConfigFile | castDetectionSensorBusAddress detected | sensor_id: ", String(configRTC.sensor[i].sensor_id), " bus_address: ", String(configRTC.sensor[i].bus_address), " parameter: ", String(configRTC.sensor[i].parameter));
        break;
      }
    }
  }

  if (!sensorFoundCastDetSensor)
  {
    Log(LogCategorySensors, LogLevelERROR, "LoggerConfigFile | castDetectionSensorBusAddress not found | sensor_id: ", String(configRTC.cast_det_sensor));
    transmitUpdateMessage(("LoggerConfigFile | castDetectionSensorBusAddress not found | sensor_id: " + String(configRTC.cast_det_sensor)).c_str(), "hyfive/ConfigError");
    hasSensorError = true;
  }
}

/**
 * @brief Enters deep sleep after measurement.
 */
void enterDeepSleepAfterMeasurement()
{
  // if (!interfaceError){interfaceRdyErrorCounter = 0;}
  float shortestWaitingTime = shortestSensorWaitingTime;
  Log(LogCategorySensors, LogLevelDEBUG, "shortestWaitingTime0: ", String(shortestWaitingTime));
  totalOperationTime += shortestWaitingTime;
  float endCalculationTime = micros();

  Log(LogCategorySensors, LogLevelDEBUG, "totalOperationTime1: ", String(totalOperationTime));
  Log(LogCategorySensors, LogLevelDEBUG, "shortestWaitingTime1: ", String(shortestWaitingTime));

  bool calculationTimeOverflow = totalOperationTime * 1000000 < endCalculationTime;
  bool calculationTimeNegative = ((shortestWaitingTime * 1000000 - micros()) / 1000000) < 0;

  Log(LogCategorySensors, LogLevelDEBUG, "calculationTimeOverflow: ", String(calculationTimeOverflow));
  Log(LogCategorySensors, LogLevelDEBUG, "calculationTimeNegative: ", String(calculationTimeNegative));

  if (calculationTimeOverflow || calculationTimeNegative)
  {
    Log(LogCategorySensors, LogLevelDEBUG, "Measuring time too long");
    espDeepSleepSec(0);
  }
  Log(LogCategorySensors, LogLevelDEBUG, "Restzeit für den Zyklus Sleep: ", String(millis()));
  Log(LogCategorySensors, LogLevelDEBUG, "Sensor deep sleep time: ", String((shortestWaitingTime * 1000000 - micros()) / 1000000));
  Log(LogCategorySensors, LogLevelDEBUG, "esp_sleep_enable_timer_wakeup: ", String(shortestWaitingTime * 1000000 - micros()));
  esp_sleep_enable_timer_wakeup(shortestWaitingTime * 1000000 - micros()); // Mikrosekunden
  esp_deep_sleep_start();
}

/**
 * @brief Checks the status of the wet sensor.
 * @return bool True if the sensor is wet, false otherwise.
 */
bool checkWetSensorStatus()
{
  Logger.sensorWakeupDetection(waterDetectionSensorBusAddress);
  Logger.startConversion(waterDetectionSensorBusAddress);

  uint64_t start_time = esp_timer_get_time() / 1000; // Start time in milliseconds
  while ((esp_timer_get_time() / 1000 - start_time) < 2000)
  {
    Logger.getInterfaceRDY(waterDetectionSensorBusAddress);
    uint32_t interfaceRDY = AdapterSensorRawValue[waterDetectionSensorBusAddress];
    if (interfaceRDY == 1)
    {
      break;
    }
    delay(10);
  }

  Logger.Measure(waterDetectionSensorBusAddress, wetDetSensorParameterNo);
  uint32_t rawValue = AdapterSensorRawValue[waterDetectionSensorBusAddress];
  float wetSensorValue = floatingPointConvert(rawValue);

  if (wetSensorValue > configRTC.wet_det_threshold)
  {
    Log(LogCategorySensors, LogLevelDEBUG, "is underwater");
    return true;
  }
  else
  {
    interfaceSleep();
    Log(LogCategorySensors, LogLevelDEBUG, "is above water");
    return false;
  }
}

/**
 * @brief Creates a configuration header for measurements.
 */
void createConfigHeader()
{

  JsonFileRead("/loggerConfig/" + findLatestConfigurationFile("/loggerConfig"));
  configureSensorsFromJson();
  configureBasicSettingsFromJson();

  File file = SD.open("/measurements/configHeader.json", FILE_APPEND);
  if (!file)
  {
    Serial.println("Error opening the file!");
    return;
  }

  StaticJsonDocument<1000> doc1;
  doc1["logger_id"] = configRTC.logger_id;
  doc1["deployment_id"] = deployment_id;
  doc1["parameter"] = "logger";
  doc1["deckunit_id"] = config.deckunit_id;
  doc1["platform_id"] = config.platform_id;
  doc1["vessel_id"] = config.vessel_id;
  doc1["vessel_name"] = config.vessel_name;
  doc1["deployment_contact_id"] = config.deployment_contact_id;
  doc1["contact_first_name"] = config.contact_first_name;
  doc1["contact_last_name"] = config.contact_last_name;

  serializeJson(doc1, file);
  file.println();

  StaticJsonDocument<3000> doc2;
  for (int sensorNumber = 0; sensorNumber < Logger.AdapterNum_Sensors; sensorNumber++)
  {

    doc2["sensor_id"] = configRTC.sensor[sensorNumber].sensor_id;
    doc2["sample_periode_multiplier"] = configRTC.sensor[sensorNumber].sample_periode_multiplier;
    doc2["sample_cast_periode_multiplier"] = configRTC.sensor[sensorNumber].sample_cast_periode_multiplier;
    doc2["bus_address"] = configRTC.sensor[sensorNumber].bus_address;
    doc2["serial_number"] = config.sensor[sensorNumber].serial_number;

    // Calibration coefficients
    JsonObject calib_coeff = doc2.createNestedObject("calib_coeff");

    calib_coeff["1"] = config.sensor[sensorNumber].calib_coeff_1;
    calib_coeff["2"] = config.sensor[sensorNumber].calib_coeff_2;
    calib_coeff["3"] = config.sensor[sensorNumber].calib_coeff_3;
    calib_coeff["4"] = config.sensor[sensorNumber].calib_coeff_4;
    calib_coeff["5"] = config.sensor[sensorNumber].calib_coeff_5;
    calib_coeff["6"] = config.sensor[sensorNumber].calib_coeff_6;
    calib_coeff["7"] = config.sensor[sensorNumber].calib_coeff_7;
    calib_coeff["8"] = config.sensor[sensorNumber].calib_coeff_8;
    calib_coeff["9"] = config.sensor[sensorNumber].calib_coeff_9;
    calib_coeff["10"] = config.sensor[sensorNumber].calib_coeff_10;

    // Sensortyp
    JsonObject sensor_type = doc2.createNestedObject("sensor_type");
    sensor_type["sensor_type_id"] = config.sensor[sensorNumber].sensor_type_id;
    sensor_type["parameter"] = configRTC.sensor[sensorNumber].parameter;
    sensor_type["long_name"] = config.sensor[sensorNumber].long_name;
    sensor_type["unit"] = config.sensor[sensorNumber].unit;
    sensor_type["manufacturer"] = config.sensor[sensorNumber].manufacturer;
    sensor_type["model"] = config.sensor[sensorNumber].model;
    sensor_type["parameter_no"] = configRTC.sensor[sensorNumber].parameter_no;

    sensor_type["accuracy"] = config.sensor[sensorNumber].accuracy;
    sensor_type["resolution"] = config.sensor[sensorNumber].resolution;

    doc2["logger_id"] = configRTC.logger_id;
    doc2["deployment_id"] = deployment_id;

    serializeJson(doc2, file);
    file.println();
  }

  file.close();
}

/**
 * @brief Gets the interface version for a specific sensor.
 * @param bus_address The I2C bus address of the sensor.
 * @return String The interface version.
 */
String interfaceVersion(int bus_address)
{
  ;
  Logger.getInterfaceVersion(bus_address, 3);
  int64_t version = AdapterSensorRawValue[bus_address];

  String sensorVersion;

  switch (version)
  {
  case 1:
    Serial.println("Bluerobotics_Bar30");
    sensorVersion = "Bluerobotics_Bar30";
    break;
  case 2:
    Serial.println("Bluerobotics_CTSYS01");
    sensorVersion = "Bluerobotics_CTSYS01";
    break;
  case 3:
    Serial.println("atlas_scientific_K0.1 / atlas_scientific_K1.0");
    sensorVersion = "atlas_scientific_K0.1 / atlas_scientific_K1.0";
    break;
  case 4:
    Serial.println("oxygen_NAU-OIW");
    sensorVersion = "oxygen_NAU-OIW";
    break;
  case 5:
    Serial.println("oxygen_NAU-YOP");
    sensorVersion = "oxygen_NAU-YOP";
    break;
  case 6:
    Serial.println("Keller_Pressure_20D");
    sensorVersion = "Keller_Pressure_20D";
    break;
  case 7:
    Serial.println("aml_ct_xchange");
    sensorVersion = "aml_ct_xchange";
    break;
  case 8:
    Serial.println("aml_ct_xchange");
    sensorVersion = "aml_ct_xchange";
    break;
  case 9:
    Serial.println("PyroPicoO2_oxycap_sub__oxycap_hs_sub");
    sensorVersion = "PyroPicoO2_oxycap_sub__oxycap_hs_sub";
    break;
  case 10:
    Serial.println("atlas_scientific_K0.1__atlas_scientific_K1.0");
    sensorVersion = "atlas_scientific_K0.1__atlas_scientific_K1.0";
    break;
  case 11:
    Serial.println("PyroPicoO2_oxycap_sub__oxycap_hs_sub");
    sensorVersion = "PyroPicoO2_oxycap_sub__oxycap_hs_sub";
    break;
  case 12:
    Serial.println("Turner_turbidity_C-Flour_TRB__Turner_phycoerythrin_C-Flour_PE");
    sensorVersion = "Turner_turbidity_C-Flour_TRB__Turner_phycoerythrin_C-Flour_PE";
    break;
  case 13:
    Serial.println("Turner_turbidity_C-Flour_TRB__Turner_phycoerythrin_C-Flour_PE");
    sensorVersion = "Turner_turbidity_C-Flour_TRB__Turner_phycoerythrin_C-Flour_PE";
    break;

  default:
    Serial.print("sensorVersion Unbekannter Sensor: ");
    Serial.println(AdapterSensorRawValue[bus_address], HEX);
    break;
  }

  return sensorVersion;
}

/**
 * @brief Checks the interface voltage for a specific sensor.
 * @param bus_address The I2C bus address of the sensor.
 */
void interfaceSensorVoltage(int bus_address)
{
  Logger.getInterfaceSensorVoltage(bus_address);
  int64_t voltage = (AdapterSensorRawValue[bus_address]);
  int64_t currentNeedVoltage = 0;

  switch (voltage)

  {
  case 1:
    Serial.println("3.3V");
    currentNeedVoltage = 0;
    break;
  case 3:
    Serial.println("3.3V - 5V");
    currentNeedVoltage = 1;
    break;
  case 5:
    Serial.println("3.3V - 12V");
    currentNeedVoltage = 2;
    break;
  case 7:
    Serial.println("3.3V - 5V - 12V");
    currentNeedVoltage = 3;
    break;
  default:
    Serial.print("voltage Unbekannter Sensor: ");
    Serial.println(AdapterSensorRawValue[bus_address], BIN);
    break;
  }

  // Update maxNeedVoltage if currentNeedVoltage is greater
  if (currentNeedVoltage > needVoltage)
  {
    needVoltage = currentNeedVoltage;
  }
}

/**
 * @brief Gets the interface parameter name for a specific sensor.
 * @param bus_address The I2C bus address of the sensor.
 * @return String The parameter name.
 */
String interfaceParameterName(int bus_address)
{
  Logger.getInterfaceParameter(bus_address);
  int64_t parameter = AdapterSensorRawValue[bus_address];

  String sensorType;

  switch (parameter)
  {
  case 1:
    Serial.println("Temperature");
    sensorType = "Temperature";
    break;
  case 2:
    Serial.println("Pressure");
    sensorType = "Pressure";
    break;
  case 3:
    Serial.println("Oxygen");
    sensorType = "Oxygen";
    break;
  case 4:
    Serial.println("Conductivity");
    sensorType = "Conductivity";
    break;
  case 5:
    Serial.println("Turbidity / Phycoerythrin");
    sensorType = "Turbidity / Phycoerythrin";
    break;
  default:
    Serial.print("sensorType Unbekannter Sensor: ");
    Serial.println(AdapterSensorRawValue[bus_address], HEX);
    break;
  }
  return sensorType;
}

/**
 * @brief Sends temperature data to a sensor interface.
 * @param bus_address The I2C bus address of the sensor.
 * @param temp The temperature value to send.
 */
void setSensorTempToInterface(uint8_t bus_address, float temp)
{
  Logger.sendTemperature(bus_address, temp);
}

/**
 * @brief Sets calibration data for a sensor interface.
 * @param bus_address The I2C bus address of the sensor.
 * @param index The index of the calibration data.
 * @param calib The calibration value to set.
 */
void setSensorCalibToInterface(uint8_t bus_address, uint8_t index, float calib)
{
  Logger.setCalib(bus_address, index, calib);
}

/**
 * @brief Sets the required voltage for sensors.
 * @param enable True to enable the voltage, false to disable.
 */
void setRequiredVoltage(bool enable)
{
  int64_t getVoltage = needVoltage;

  if (enable)
  {
    switch (getVoltage)
    {
    case 0:
      break;
    case 1:
      Serial.println("enable 5V");
      enable5V();
      break;
    case 2:
      Serial.println("enable 12V");
      enable12V();
      break;
    case 3:
      Serial.println("enable 5V | 12V");
      enable5V();
      enable12V();
      break;
    default:
      Serial.print("enable setVoltage Unbekannter Sensor: ");
      break;
    }
  }
  else
  {
    switch (getVoltage)
    {
    case 0:
      Serial.println("disabl 3.3V");
      break;
    case 1:
      Serial.println("disabl 5V");
      disable5V();
      break;
    case 2:
      Serial.println("disabl 12V");
      disable12V();
      break;
    case 3:
      Serial.println("disabl 5V | 12V");
      disable5V();
      disable12V();
      break;
    default:
      Serial.print("disable setVoltage Unbekannter Sensor: ");
      break;
    }
  }
}

/**
 * @brief Updates sampling intervals based on cast status.
 * @param useFastSampling True to use fast sampling, false otherwise.
 */
void updateSamplingIntervals(bool useFastSampling)
{
  for (int sensorNumber = 0; sensorNumber < numberOfActiveSensors; ++sensorNumber)
  {
    if (useFastSampling)
    {
      intervalSensorArray[sensorNumber] = configRTC.sample_cast_periode *
                                          configRTC.sensor[sensorNumber].sample_cast_periode_multiplier;
    }
    else
    {
      intervalSensorArray[sensorNumber] = configRTC.sample_periode *
                                          configRTC.sensor[sensorNumber].sample_periode_multiplier;
    }
  }
}

/**
 * @brief Updates the wet sensor interval.
 */
void updateWetSensorInterval()
{
  for (int sensorNumber = 0; sensorNumber < numberOfActiveSensors; ++sensorNumber)
  {
    if (configRTC.sensor[sensorNumber].sensor_id == configRTC.wet_det_sensor)
    {
      intervalSensorArray[sensorNumber] = 1;
      break;
    }
  }
}

/**
 * @brief Checks for dry condition.
 * @return bool True if dry condition is met, false otherwise.
 */
bool checkDryCondition()
{
  for (int i = 0; i < 100; i++)
  {
    Logger.getInterfaceRDY(dryDetectionSensorBusAddress);
    uint32_t interfaceRDY = AdapterSensorRawValue[dryDetectionSensorBusAddress];
    if (interfaceRDY == 1)
    {
      break;
    }
    delay(10); // interfaceRDY
  }

  Logger.Measure(dryDetectionSensorBusAddress, dryDetSensorParameterNo);
  uint32_t rawValue = AdapterSensorRawValue[dryDetectionSensorBusAddress];
  float drySensorValue = floatingPointConvert(rawValue);

  Log(LogCategorySensors, LogLevelDEBUG, "drySensorValue: ", String(drySensorValue));
  Log(LogCategorySensors, LogLevelDEBUG, "configRTC.dry_det_threshold: ", String(configRTC.dry_det_threshold));

  if (drySensorValue <= configRTC.dry_det_threshold)
  {
    if (thresholdValuewaterDetection && (detectionThresholdValue < configRTC.dry_det_verify_delay))
    {
      for (int sensorNumber = 0; sensorNumber < numberOfActiveSensors; ++sensorNumber)
      {
        if (configRTC.sensor[sensorNumber].sensor_id == configRTC.wet_det_sensor)
        {
          configRTC.sample_periode = 1;
          intervalSensorArray[sensorNumber] = 1;
        }
      }

      updateWetSensorInterval();
      ++detectionThresholdValue;
      Log(LogCategorySensors, LogLevelDEBUG, "Over water counter: ", String(detectionThresholdValue));
      return true;
    }
    else
    {
      detectionThresholdValue = 0;
      thresholdValuewaterDetection = false;
      return false;
    }
  }
  else
  {
    detectionThresholdValue = 0;
  }
  return thresholdValuewaterDetection;
}

/**
 * @brief Wakes up the sensor interface.
 */
void interfaceWakeup()
{
  for (int i = 0; i < SensorArraySize; i++)
  {
    if (AdapterSensorTypeID[configRTC.sensor[i].bus_address] != 0)
    {
      Logger.sensorWakeup(i);
    }
  }
  delay(longestSensorWakeupTime);
}

/**
 * @brief Puts the sensor interface to sleep.
 */
void interfaceSleep()
{
  for (int i = 0; i < 33; i++)
  {
    Logger.sensorSleep(i);
  }
}

/**
 * @brief Initiates underwater mode.
 */
void initiateUnderwaterMode()
{
  setRequiredVoltage(true);
  createOrClearSampleFile();
  writeDeploymentIdToFile();
  createConfigHeader();

  if (!isLoggerSubmerged)
  {
    Log(LogCategoryMeasurement, LogLevelINFO, "Underwater measurement begin");
  }

  totalMeasurementCount = 0;
  isLoggerSubmerged = true;
  Logger.sensorWakeupAll();
  espDeepSleepSec(0);
}

/**
 * @brief Checks if water is detected.
 * @return bool True if water is detected, false otherwise.
 */
bool isWaterDetected()
{
  Serial.println("Starte Messung...");
  tStart = millis();

  if (thresholdValuewaterDetection)
  {
    configRTC.sample_periode = saveSamplePeriodeToResetAfterUnderwaterMeasurementsEnd;
    return true;
  }

  if (!thresholdValuewaterDetection)
  {
    Logger.sensorWakeupDetection(waterDetectionSensorBusAddress);
    Logger.startConversion(waterDetectionSensorBusAddress);

    uint64_t start_time = esp_timer_get_time() / 1000; // Startzeit in Millisekunden
    while ((esp_timer_get_time() / 1000 - start_time) < 2000)
    // for (int i = 0; i < 100; i++)
    {
      Logger.getInterfaceRDY(waterDetectionSensorBusAddress);
      uint32_t interfaceRDY = AdapterSensorRawValue[waterDetectionSensorBusAddress];
      if (interfaceRDY == 1)
      {
        interfaceRdyErrorCounter = 0;
        interfaceErrorSensorId = 0;
        Log(LogCategorySensors, LogLevelDEBUG, "wet_det_threshold interfaceRDY == OK");
        break;
      }

      if (interfaceRDY == 2)
      {
        Log(LogCategorySensors, LogLevelDEBUG, "wet_det_threshold interfaceRDY == 2");
        Logger.interfaceSoftwareReset(waterDetectionSensorBusAddress);
        interfaceRdyErrorCounter++;
        sensorCalibToInterfaceIfRdyErrorCounter = 4;
        espDeepSleepSec(0);
        // break;
      }

      delay(10); // interfaceRDY
    }

    Logger.Measure(waterDetectionSensorBusAddress, wetDetSensorParameterNo);
    uint32_t rawValue = AdapterSensorRawValue[waterDetectionSensorBusAddress];
    float wetSensorValue = floatingPointConvert(rawValue);

    Log(LogCategorySensors, LogLevelDEBUG, "wetSensorValue: ", String(wetSensorValue));
    Log(LogCategorySensors, LogLevelDEBUG, "configRTC.wet_det_threshold: ", String(configRTC.wet_det_threshold));

    if (wetSensorValue >= configRTC.wet_det_threshold)
    {
      configRTC.sample_periode = saveSamplePeriodeToResetAfterUnderwaterMeasurementsEnd;
      detectionThresholdValue = 0;
      thresholdValuewaterDetection = true;
      initiateUnderwaterMode();
      return true;
    }
    else
    {
      interfaceSleep();
      return false;
    }
  }
  else
  {
    interfaceSleep();
    return false;
  }
}

/**
 * @brief Terminates underwater mode.
 */
void terminateUnderwaterMode()
{
  interfaceSleep();
  moveMeasurementAndData();
  bootAttemptCount = 0;
  totalOperationTime = 0;
  memset(lastSensorMeasurementTime, 0, sizeof(lastSensorMeasurementTime));
  isLoggerSubmerged = false;
  setRequiredVoltage(false);
  configRTC.sample_periode = saveSamplePeriodeToResetAfterUnderwaterMeasurementsEnd;
  Log(LogCategoryMeasurement, LogLevelINFO, "Underwater measurement end: ",
      "logger_id: ", String(configRTC.logger_id),
      " deployment_id: ", String(deployment_id));

  waitAfterUnderwaterMeasurementTimeNow = getCurrentTimeFromRTC() + waitAfterUnderwaterMeasurementTime;
  Log(LogCategoryMeasurement, LogLevelDEBUG, "getCurrentTimeFromRTC()", String(getCurrentTimeFromRTC()));
  Log(LogCategoryMeasurement, LogLevelDEBUG, "waitAfterUnderwaterMeasurementTimeNow", String(waitAfterUnderwaterMeasurementTimeNow));

  inactiveMeasurement();
}

/**
 * @brief Checks the wet sensor threshold.
 */
void checkWetSensorThreshold()
{
  if (!isPowerSupplyConnected())
  {
    if (interfaceRdyErrorCounter > 0 || sensorCalibToInterfaceIfRdyErrorCounter > 0)
    {
      validateAndLoadConfig();
      sensorCalibToInterface();
      sensorCalibToInterfaceIfRdyErrorCounter--;
      Log(LogCategorySensors, LogLevelDEBUG, "sensorCalibToInterfaceIfRdyErrorCounter ", String(sensorCalibToInterfaceIfRdyErrorCounter));
    }
    while (isWaterDetected())
    {
      if (interfaceRdyErrorCounter > 0 || sensorCalibToInterfaceIfRdyErrorCounter > 0)
      {
        validateAndLoadConfig();
        sensorCalibToInterface();
      }

      if ((setBeginDeployment) && (detectionThresholdValue == 0))
      {
        setBeginDeployment = false;
        ledControl(LedMode::loggerDetectsBeginOfDeployment);
        Serial.println("--------------------------------------loggerDetectsBeginOfDeployment");
      }

      if (detectionThresholdValue > 0)
      {
        ledControl(LedMode::loggerDetectsEndOfDeployment);
        setBeginDeployment = true;
        sensorStartDone = false;
        Serial.println("--------------------------------------loggerDetectsEndOfDeployment");
      }

      if (ledSensorTrue && sensorStartDone)
      {
        ledSensorTrue = false;
        if (detectionThresholdValue == 0)
        {
          ledControl(LedMode::duringDeployment);
          Serial.println("--------------------------------------duringDeployment");
        }

      }

      performUnderWaterOperations();

      float shortestWaitingTime = calculateShortestSensorWaitTime();
      shortestSensorWaitingTime = shortestWaitingTime;
      totalOperationTime += shortestWaitingTime;

      tEnd = millis(); // Endzeit in ms
      unsigned long diff = tEnd - tStart;
      Serial.print("Dauer: ");
      Serial.print(diff);
      Serial.println(" ms");

      Serial.print("shortestWaitingTime: ");
      Serial.print(shortestWaitingTime);
      Serial.println(" sec");

      shortestWaitingTime = (shortestWaitingTime * 1000) - diff;

      Serial.print("shortestWaitingTimeForDealy: ");
      Serial.print(shortestWaitingTime);
      Serial.println(" sec");

      while (!ledMeasurementsOff.load())
      {
        delay(10);
      }

      if (sensorStartDone)
      {
        if (shortestWaitingTime > 500)
        {
          Serial.print("enterDeepSleepAfterMeasurement");
          enterDeepSleepAfterMeasurement();
        }
      }

      if (shortestWaitingTime > 0)
      {
        delay(shortestWaitingTime);
      }
    }

    if (isLoggerSubmerged)
    {
      terminateUnderwaterMode();
    }
  }
}

std::atomic<bool> ledMeasurementsOff(true);

/**
 * @brief Performs underwater operations.
 */
void performUnderWaterOperations()
{
  // LEDLoggerDetectsBeginOfDeployment();
  startConversionformUnderWaterOperations();
  checkDryCondition();
  updateSamplingIntervals(configRTC.sample_cast_enable && performSampleCast());
  updateSensorMeasurements();
  writeMeasurementDataToFile();
}

/**
 * @brief Checks sensor availability and performs initial setup.
 */
void sensorAvailability()
{
  for (int i = 0; i < SensorArraySize; i++)
  {
    if (AdapterSensorTypeID[configRTC.sensor[i].bus_address] != 0)
    {
      Logger.getSensorWakeupTime(configRTC.sensor[i].bus_address);
      uint16_t sensorWakeupTime = AdapterSensorRawValue[configRTC.sensor[i].bus_address];
      if (sensorWakeupTime > longestSensorWakeupTime)
      {
        longestSensorWakeupTime = sensorWakeupTime;
      }
    }
  }

  Log(LogCategorySensors, LogLevelDEBUG, "longestSensorWakeupTime: ", String(longestSensorWakeupTime));

  detectConnectedSensorDevices();
  detectDryWetCastSensors();

  Logger.sensorWakeupDetection(waterDetectionSensorBusAddress);
  Logger.startConversion(waterDetectionSensorBusAddress);

  if (!hasSensorError)
  {
    performInitialMeasurement();
  }

  if (hasSensorError && isFirstBoot)
  {
    generalError();
  }

  if (hasSensorError)
  {
    performPeriodicConfigUpdate();
    isLoggerBusy = false;
    generalError();
  }
}