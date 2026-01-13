/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Implementation of I2C master communication functions
 */

#ifndef I2C_MASTER_H
#define I2C_MASTER_H_

#include <Wire.h>

class I2C_Master
{
public:
  I2C_Master();
  void Init(TwoWire &wirePort, uint8_t *DeviceVersions, int64_t *SensorValue1, int64_t *SensorValue2, int SDA_Pin, int SCL_Pin);
  uint8_t Scan_Bus();
  uint8_t getVersion(uint8_t address, uint16_t id);
  uint8_t getSensorVoltage(uint8_t address);
  uint8_t getParameter(uint8_t address);
  uint8_t getRDY(uint8_t address);
  uint8_t getFwVersion(uint8_t address);
  uint8_t getCalibrated(uint8_t address);
  uint16_t getSensorWakeupTime(uint8_t address);
  int64_t getSensorValue_1_Calc(uint8_t address);
  int64_t getSensorValue_1_Calc_RAW(uint8_t address);
  int64_t getSensorValue_2_Calc(uint8_t address);
  int64_t getSensorValue_2_Calc_RAW(uint8_t address);
  void startConversion(uint8_t address);
  void sensorSleep(uint8_t address);
  void interfaceSoftwareReset(uint8_t address);
  void sensorWakeup(uint8_t address);
  void sendTemperature(uint8_t address, float temperature);
  void setCalib(uint8_t address, uint8_t index, float calib);

  void begin_I2C();

private:
  uint8_t Write(uint8_t command, uint8_t address);
  uint8_t WriteNByte(uint8_t command, const uint8_t *data, size_t quantity, uint8_t address);
  void Read(uint8_t address, uint8_t *answer, uint8_t length);
  uint8_t WriteRead(uint8_t command, uint8_t address, uint8_t *answer, uint8_t length);

  int64_t getValue(uint8_t command, uint8_t address);
  TwoWire *_i2cPort;
  uint8_t *DeviceTypeID;
  int64_t *SensorValue_1;
  int64_t *SensorValue_2;
  int SDA_Pin; // 4; // 4 --> HyFiveBoard // 21; // 21 --> Esp32 Devkit
  int SCL_Pin; // 5; // 5 --> HyFiveBoard // 22; // 22 --> Esp32 Devk
  uint32_t BusFrequency = 100000;
};

bool sensorValueErrorFunction();

#endif