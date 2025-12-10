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

#include "../../src/DebuggingSDLog.h"
#include "../../src/DeepSleep.h"
#include "../../src/Led.h"
#include "../../src/SystemVariables.h"
#include "../../src/loggerConfig.h"
#include <Arduino.h>
#include <Device_CMD.cpp>
#include <I2C_Master.h>
#include <Wire.h>

I2C_Master::I2C_Master()
{
}

void I2C_Master::Init(TwoWire &wirePort,
                      uint8_t *DeviceVersions,
                      int64_t *SensorValue_1,
                      int64_t *SensorValue_2,
                      int SDA_Pin,
                      int SCL_Pin)
{

  this->_i2cPort = &wirePort;
  this->SDA_Pin = SDA_Pin;
  this->SCL_Pin = SCL_Pin;

  this->DeviceTypeID = DeviceVersions;
  this->SensorValue_1 = SensorValue_1;
  this->SensorValue_2 = SensorValue_2;

  _i2cPort->begin(this->SDA_Pin, this->SCL_Pin, this->BusFrequency);
}

void I2C_Master::begin_I2C()
{
  _i2cPort->begin(this->SDA_Pin, this->SCL_Pin, this->BusFrequency);
}

uint8_t I2C_Master::getVersion(uint8_t address, uint16_t id)
{
  uint8_t DeviceTypeID[4];

  this->WriteRead(CMD_GETVER, address, DeviceTypeID, 4);

  if (id == 0)
  {
    return DeviceTypeID[0];
  }
  else if (id == 1)
  {
    return DeviceTypeID[1];
  }
  else if (id == 2)
  {
    return DeviceTypeID[2];
  }
  else if (id == 3)
  {
    return DeviceTypeID[3];
  }
  else
  {
    return 0xFF;
  }
}

uint8_t I2C_Master::getSensorVoltage(uint8_t address)
{
  uint8_t DeviceTypeID, length;
  length = 1;

  this->WriteRead(CMD_GET_SENSORVOLTAGE, address, &DeviceTypeID, 1);

  return DeviceTypeID;
}

uint8_t I2C_Master::getParameter(uint8_t address)
{
  uint8_t DeviceTypeID, length;
  length = 1;

  this->WriteRead(CMD_GET_PARAMETER, address, &DeviceTypeID, 1);

  return DeviceTypeID;
}

uint8_t I2C_Master::getRDY(uint8_t address)
{
  uint8_t DeviceTypeID, length;
  length = 1;

  this->WriteRead(CMD_GET_RDY, address, &DeviceTypeID, 1);

  return DeviceTypeID;
}

uint8_t I2C_Master::getCalibrated(uint8_t address)
{
  uint8_t DeviceTypeID, length;
  length = 1;

  this->WriteRead(CMD_GET_CALIBRATED, address, &DeviceTypeID, 1);

  return DeviceTypeID;
}

uint8_t I2C_Master::getFwVersion(uint8_t address)
{
  uint8_t fwVersion;

  this->WriteRead(CMD_GET_FW_VERSION, address, &fwVersion, 1);

  return fwVersion;
}

uint16_t I2C_Master::getSensorWakeupTime(uint8_t address)
{
  uint8_t buffer[2];
  uint16_t wakeupTime;

  this->WriteRead(CMD_GET_SENSOR_WAKEUP_TIME, address, buffer, 2);

  wakeupTime = (buffer[0] << 8) | buffer[1];

  return wakeupTime;
}

/*
uint8_t I2C_Master::getSensorType(uint8_t address){
        uint8_t SensorType, length;
        length = 1;

        this->WriteRead(CMD_GETSENSOR,address, &SensorType, length);

        return SensorType;
}*/

int64_t I2C_Master::getSensorValue_1_Calc(uint8_t address)
{
  int64_t ret;
  ret = this->getValue(CMD_GETVALUE1, address);
  return ret;
}

int64_t I2C_Master::getSensorValue_1_Calc_RAW(uint8_t address)
{
  int64_t ret;
  ret = this->getValue(CMD_GETVALUE2, address);
  return ret;
}

int64_t I2C_Master::getSensorValue_2_Calc(uint8_t address)
{
  int64_t ret;
  ret = this->getValue(CMD_GETRAWVALUE1, address);
  return ret;
}

int64_t I2C_Master::getSensorValue_2_Calc_RAW(uint8_t address)
{
  int64_t ret;
  ret = this->getValue(CMD_GETRAWVALUE2, address);
  return ret;
}

// int64_t I2C_Master::getValue(uint8_t command, uint8_t address)
//{
//	uint8_t SensorValue[8];
//	int64_t ret;
//	this->WriteRead(command, address, SensorValue, 8);
//
//	ret = ((int64_t)SensorValue[7]);
//	ret |= (((int64_t)SensorValue[6]) << 8);
//	ret |= (((int64_t)SensorValue[5]) << 16);
//	ret |= (((int64_t)SensorValue[4]) << 24);
//	ret |= (((int64_t)SensorValue[3]) << 32);
//	ret |= (((int64_t)SensorValue[2]) << 40);
//	ret |= (((int64_t)SensorValue[1]) << 48);
//	ret |= (((int64_t)SensorValue[0]) << 56);
//
//	return ret;
// }
bool sensorValueError = false;
int64_t I2C_Master::getValue(uint8_t command, uint8_t address)
{
  uint8_t SensorValue[8];
  int64_t ret;

  const int MAX_RETRIES = 3;
  for (int retry = 0; retry < MAX_RETRIES; retry++)
  {
    if (this->WriteRead(command, address, SensorValue, 8) == 0)
    {
      int64_t ret = ((int64_t)SensorValue[7]);
      ret |= (((int64_t)SensorValue[6]) << 8);
      ret |= (((int64_t)SensorValue[5]) << 16);
      ret |= (((int64_t)SensorValue[4]) << 24);
      ret |= (((int64_t)SensorValue[3]) << 32);
      ret |= (((int64_t)SensorValue[2]) << 40);
      ret |= (((int64_t)SensorValue[1]) << 48);
      ret |= (((int64_t)SensorValue[0]) << 56);

      // Serial.println("START-getValue");
      // Serial.println(SensorValue[7], HEX);
      // Serial.println(SensorValue[6], HEX);
      // Serial.println(SensorValue[5], HEX);
      // Serial.println(SensorValue[4], HEX);
      // Serial.println(SensorValue[3], HEX);
      // Serial.println(SensorValue[2], HEX);
      // Serial.println(SensorValue[1], HEX);
      // Serial.println(SensorValue[0], HEX);
      // Serial.println("ENDE-getValue");

      sensorValueError = false;
      return ret;
    }
  }

  // if (address == waterDetectionSensorBusAddress || address == dryDetectionSensorBusAddress || address == castDetectionSensorBusAddress)
  // {
  //   Log(LogCategorySensors, LogLevelERROR, " I2C|detection Sensor not found, bus address: ", String(address));
  //   generalError();
  // }
  return 0;
}

/**
 * @brief Überprüft, ob ein Fehler bei der Sensorwertmessung aufgetreten ist.
 *
 * Diese Funktion prüft, ob bei der letzten Sensorwertmessung ein Fehler aufgetreten ist.
 * Sie kann verwendet werden, um die Zuverlässigkeit der Sensordaten zu gewährleisten.
 *
 * @return true wenn ein Fehler aufgetreten ist, false wenn die Messung erfolgreich war.
 */
bool sensorValueErrorFunction()
{
  if (sensorValueError)
  {
    return true;
  }
  else
  {
    return false;
  }
}

void I2C_Master::startConversion(uint8_t address)
{
  ;
  this->Write(CMD_CONVERT, address);
}

void I2C_Master::sensorSleep(uint8_t address)
{
  ;
  //Serial.println("CMD_SENSOR_SLEEP");
  this->Write(CMD_SENSOR_SLEEP, address);
}

void I2C_Master::interfaceSoftwareReset(uint8_t address)
{
  ;
  this->Write(CMD_SOFTWARE_RESET, address);
}

void I2C_Master::sensorWakeup(uint8_t address)
{
  ;
  //Serial.println("CMD_SENSOR_WAKEUP");
  this->Write(CMD_SENSOR_WAKEUP, address);
}

void I2C_Master::sendTemperature(uint8_t address, float temperature)
{
  union
  {
    float float_variable;
    unsigned char temp_array[4];
  } u;
  u.float_variable = (float)temperature;
  uint8_t data[4];
  data[0] = u.temp_array[3];
  data[1] = u.temp_array[2];
  data[2] = u.temp_array[1];
  data[3] = u.temp_array[0];
  this->WriteNByte(CMD_SET_TEMP, data, 4, address);
}

void I2C_Master::setCalib(uint8_t address, uint8_t index, float calib)
{
  union
  {
    float float_variable;
    unsigned char temp_array[4];
  } u;
  u.float_variable = (float)calib;
  uint8_t data[5];
  data[0] = index;
  data[1] = u.temp_array[3];
  data[2] = u.temp_array[2];
  data[3] = u.temp_array[1];
  data[4] = u.temp_array[0];
  this->WriteNByte(CMD_SET_CALIB, data, 5, address);
}

uint8_t I2C_Master::Write(uint8_t command, uint8_t address)
{
  uint8_t ret;
  _i2cPort->beginTransmission(address);
  _i2cPort->write(command);
  ret = _i2cPort->endTransmission();
  return ret;
}

// size_t TwoWire::write(const uint8_t *data, size_t quantity)
uint8_t I2C_Master::WriteNByte(uint8_t command, const uint8_t *data, size_t quantity, uint8_t address)
{
  uint8_t ret;
  uint8_t sendData[10];
  if (quantity > 9)
    return 0;
  sendData[0] = command;
  for (int i = 0; i < quantity; i++)
  {
    sendData[i + 1] = data[i];
  }

  // Serial.println("START-WriteNByte");
  // Serial.println(sendData[0], HEX);
  // Serial.println(sendData[1], HEX);
  // Serial.println(sendData[2], HEX);
  // Serial.println(sendData[3], HEX);
  // Serial.println(sendData[4], HEX);
  // Serial.println(sendData[5], HEX);
  // Serial.println(sendData[6], HEX);
  // Serial.println(sendData[7], HEX);
  // Serial.println("END-WriteNByte");

  _i2cPort->beginTransmission(address);
  _i2cPort->write(sendData, quantity + 1);
  ret = _i2cPort->endTransmission();
  return ret;
}

void I2C_Master::Read(uint8_t address, uint8_t *answer, uint8_t length)
{
  _i2cPort->requestFrom(address, length);
  answer[0] = _i2cPort->read();
  for (int i = 1; i < length; i++)
  {
    answer[i] = _i2cPort->read(); // << (8*i);
  }
}

uint8_t I2C_Master::WriteRead(uint8_t command, uint8_t address, uint8_t *answer, uint8_t length)
{
  uint8_t ret;
  ret = this->Write(command, address);
  
  if (ret != 0) 
  {return ret;}

  this->Read(address, answer, length);

  return ret;
}

uint8_t I2C_Master::Scan_Bus()
{
  uint8_t address, answer, DeviceCount;
  answer = 6;
  DeviceCount = 0;

  for (address = 0; address < MAX_SENSOR_CREDENTIALS; address++)
  { // Ersetzen durch NUM_SENSORS
    answer = this->Write(CMD_PING, address);
    if (answer == 0)
    { // Success
      this->DeviceTypeID[address] = this->getVersion(address, 3);
      DeviceCount++;
    }
    else if (answer == 1)
    { // data too long to fit in transmit buffer
    }
    else if (answer == 2)
    { // received NACK on transmit of address
    }
    else if (answer == 3)
    { // received NACK on transmit of data
    }
    else if (answer == 4)
    { // other error
    }
    else if (answer == 5)
    { // other error
    }
    delayMicroseconds(500);
  }
  // Serial.print("DeviceCount = ");
  // Serial.println(DeviceCount);
  return DeviceCount;
}