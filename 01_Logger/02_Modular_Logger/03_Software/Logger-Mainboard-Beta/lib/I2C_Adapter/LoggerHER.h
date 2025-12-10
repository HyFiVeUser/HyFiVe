/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Implementation of logger functions
 */

#ifndef LOGGERHER_H
#define LOGGERHER_H
#include "Arduino.h"
// #include "config.h"
#include "I2C_Master.h"
#include <FS.h>
#include <SD.h>
#include <time.h>
// #include <ESP32Time.h>
#include "../../src/SensorManagement.h"
#include "../../src/SystemVariables.h"
#include "../../src/loggerConfig.h"
#include "../../src/loggerConfigValidation.h"
#include <Wire.h>

enum LoggerState
{
  inSituLogger,
  dryLoggerEmpty,
  dryLoggerwFiles,
  dryLoggerwUpdate,
  testing
};
class LoggerHER
{
public:
  LoggerHER();
  void Init(TwoWire &wirePort,
            uint8_t *DeviceVersions,
            uint8_t *Config_Value_Type,
            int64_t *SensorRawValue,
            int64_t *SensorCalcValue,
            uint8_t bootCountt,
            int SD_CS,
            int SDA_Pin,
            int SCL_Pin,
            SPIClass *spi_interface);

  void Measure_All(uint8_t Value_Nr);
  void Measure(uint8_t address, uint8_t Value_Nr);
  void getInterfaceVersion(uint8_t address, uint16_t id);
  void getInterfaceSensorVoltage(uint8_t address);
  void getInterfaceParameter(uint8_t address);
  void getInterfaceRDY(uint8_t address);
  void getCalibrated(uint8_t address);
  void getSensorWakeupTime(uint8_t address);
  void getFwVersion(uint8_t address);
  void startConversion32();
  void startConversionAll(uint8_t address);
  void startConversion(uint8_t address);
  void sensorSleep(uint8_t address);
  void interfaceSoftwareReset(uint8_t address);
  void sensorWakeup(uint8_t address);
  void sensorWakeupAll();
  void sensorWakeupDetection(uint8_t address);
  void sendTemperature(uint8_t address, float temperature);
  void setCalib(uint8_t address, uint8_t index, float calib);

  int64_t getAdapterSensorRawValue(uint8_t address);
  int64_t getAdapterSensorCalcValue(uint8_t address);

  void setState(uint8_t in);
  void setChipid(int64_t id);
  void setFileName(char *name);
  void writeHeader();
  uint8_t SaveToCard();
  void setID(uint8_t id);
  void setDiveId(uint8_t dive);
  void setScheduler_BootCount(uint32_t Bootcount);
  void begin_I2C();
  int64_t getChipid();
  uint8_t getState();
  uint8_t getID();
  uint8_t getDiveId();
  uint32_t getScheduler_BootCount();
  const char *getFileName();

  uint8_t AdapterNum_Sensors;

private:
  TwoWire *_i2cPort;
  I2C_Master AdapterBus;
  SPIClass *spi;

  // ESP32Time rtc;

  struct tm timeinfo;

  uint8_t AdapterBuffer[8];
  uint8_t AdapterBuffer_Len = 8;

  // uint8_t   AdapterNum_Sensors;
  uint8_t *AdapterDeviceVersions;
  int64_t *AdapterSensorValue;
  int64_t *AdapterSensorRawValue;
  int64_t *AdapterSensorCalcValue;

  uint8_t *AdapterConfig_Value_Type;
  uint8_t *AdapterConfig_Delay;

  const int Max_Num_Sensors = MAX_SENSOR_CREDENTIALS;
  int sd_cs;
  int64_t chipid;
  uint8_t id;
  uint8_t diveID;
  uint32_t Scheduler_Bootcount;
  uint8_t state;
  char currentFile[23];
};

extern LoggerHER Logger;

#endif
