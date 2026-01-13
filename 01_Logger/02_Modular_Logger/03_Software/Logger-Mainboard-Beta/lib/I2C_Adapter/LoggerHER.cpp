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

#include <Wire.h>

#include "LoggerHER.h"
LoggerHER Logger;
enum SensorValueType
{
  reserved,
  Type_uint8,
  Type_uint16,
  Type_uint32,
  Type_uint64,
  Type_int8,
  Type_int16,
  Type_int32,
  Type_int64,
  Type_float,
};

LoggerHER::LoggerHER() {}

void LoggerHER::Init(TwoWire &wirePort, uint8_t *DeviceVersions, uint8_t *Config_Value_Type, int64_t *SensorRawValue, int64_t *SensorCalcValue, uint8_t bootCount, int SD_CS, int SDA_Pin, int SCL_Pin, SPIClass *spi_interface)
{

  uint8_t Scan_NumSensors;

  this->sd_cs = SD_CS;
  this->spi   = spi_interface;

  // Config.h NUMSENSORS erzeugt fehler, auch wenn configRTC.h in cpp included ist
  this->AdapterNum_Sensors = configRTC.num_sensors; // Num_Sensors;
  this->_i2cPort           = &wirePort;

  this->chipid         = ESP.getEfuseMac();
  this->state          = dryLoggerEmpty;
  this->currentFile[0] = 0;

  this->AdapterDeviceVersions    = DeviceVersions;
  this->AdapterConfig_Value_Type = Config_Value_Type;

  // this->AdapterSensorTypes	= SensorTypes;
  this->AdapterSensorRawValue  = SensorRawValue;
  this->AdapterSensorCalcValue = SensorCalcValue;

  this->AdapterBus.Init(wirePort, this->AdapterDeviceVersions, this->AdapterSensorRawValue, this->AdapterSensorCalcValue, SDA_Pin, SCL_Pin);

  if (bootCount < 2)
  {
    this->state     = dryLoggerEmpty;
    Scan_NumSensors = this->AdapterBus.Scan_Bus();
    if (this->AdapterNum_Sensors != Scan_NumSensors)
    {
      // Serial.println(AdapterNum_Sensors);
      // Serial.println(Scan_NumSensors);
      // Serial.println("Logger.Init --> Sensoranzahl am Bus stimmt nicht mit Konfiguration überein");
      this->AdapterNum_Sensors = Scan_NumSensors;
    }
    else
    {
      // Serial.println("Logger.Init --> Korrekte Anzahl an Sensoren vorhanden");
    }
  }
}

void LoggerHER::begin_I2C() { this->AdapterBus.begin_I2C(); }

void LoggerHER::Measure_All(uint8_t Value_Nr)
{
  uint8_t i;
  for (i = 0; i < Max_Num_Sensors; i++)
  {
    if (this->AdapterDeviceVersions[i] == 0)
    {
      i = 33;
      break;
    }
    else
    {
      if (Value_Nr == 1)
      {
        this->Measure(i, Value_Nr);
      }
      else if (Value_Nr == 2)
      {
        this->Measure(i, Value_Nr);
      }
    }
  }
}

void LoggerHER::Measure(uint8_t address, uint8_t Value_Nr)
{
  if (Value_Nr == 1)
  {
    this->AdapterSensorRawValue[address] = this->AdapterBus.getSensorValue_1_Calc(address);
  }
  else if (Value_Nr == 2)
  {
    this->AdapterSensorRawValue[address] = this->AdapterBus.getSensorValue_1_Calc_RAW(address);
  }
  else if (Value_Nr == 3)
  {
    this->AdapterSensorRawValue[address] = this->AdapterBus.getSensorValue_2_Calc(address);
  }
  else if (Value_Nr == 4)
  {
    this->AdapterSensorRawValue[address] = this->AdapterBus.getSensorValue_2_Calc_RAW(address);
  }
}

void LoggerHER::getInterfaceVersion(uint8_t address, uint16_t id) { this->AdapterSensorRawValue[address] = this->AdapterBus.getVersion(address, id); }

void LoggerHER::getInterfaceSensorVoltage(uint8_t address) { this->AdapterSensorRawValue[address] = this->AdapterBus.getSensorVoltage(address); }

void LoggerHER::getInterfaceParameter(uint8_t address) { this->AdapterSensorRawValue[address] = this->AdapterBus.getParameter(address); }

void LoggerHER::getInterfaceRDY(uint8_t address) { this->AdapterSensorRawValue[address] = this->AdapterBus.getRDY(address); }

void LoggerHER::getCalibrated(uint8_t address) { this->AdapterSensorRawValue[address] = this->AdapterBus.getCalibrated(address); }

void LoggerHER::getSensorWakeupTime(uint8_t address) { this->AdapterSensorRawValue[address] = this->AdapterBus.getSensorWakeupTime(address); }

void LoggerHER::getFwVersion(uint8_t address) { this->AdapterSensorRawValue[address] = this->AdapterBus.getFwVersion(address); }

void LoggerHER::startConversionAll(uint8_t address) { this->AdapterBus.startConversion(address); }

void LoggerHER::startConversion32()
{
  uint8_t i;
  for (i = 0; i < Max_Num_Sensors; i++)
  {
    this->AdapterBus.startConversion(i);
  }
  delay(100);
}

void LoggerHER::startConversion(uint8_t address)
{
  this->AdapterBus.startConversion(address);
  delay(100);
}

void LoggerHER::sensorSleep(uint8_t address)
{
  this->AdapterBus.sensorSleep(address);
  delay(10);
}

void LoggerHER::interfaceSoftwareReset(uint8_t address)
{
  this->AdapterBus.interfaceSoftwareReset(address);
  Serial.print("interfaceSoftwareReset: ");
  Serial.println(address);
  delay(10);
}
void LoggerHER::sensorWakeupAll()
{
  for (int i = 0; i < 33; i++)
  {
    {
      this->AdapterBus.sensorWakeup(i);
    }
  }
  Serial.print("sensorWakeupAll_longestSensorWakeupTime: ");
  Serial.print(longestSensorWakeupTime);
  Serial.println("");
  delay(longestSensorWakeupTime);
}

void LoggerHER::sensorWakeupDetection(uint8_t address)
{
  Logger.getSensorWakeupTime(address);
  uint16_t sensorWakeupDetectionTime = AdapterSensorRawValue[address];

  this->AdapterBus.sensorWakeup(address);
  delay(sensorWakeupDetectionTime);
  // Serial.print("address: ");
  // Serial.print(address);
  // Serial.println("");
  // Serial.print("sensorWakeupDetectionTime: ");
  // Serial.print(sensorWakeupDetectionTime);
  // Serial.println("");
}

void LoggerHER::sensorWakeup(uint8_t address)
{
  this->AdapterBus.sensorWakeup(address);
  delay(longestSensorWakeupTime);
  Serial.print("address: ");
  Serial.print(address);
  Serial.println("");
  Serial.print("longestSensorWakeupTime: ");
  Serial.print(longestSensorWakeupTime);
  Serial.println("");
}

void LoggerHER::sendTemperature(uint8_t address, float temperature) { this->AdapterBus.sendTemperature(address, temperature); }

void LoggerHER::setCalib(uint8_t address, uint8_t index, float calib) { this->AdapterBus.setCalib(address, index, calib); }

int64_t LoggerHER::getAdapterSensorRawValue(uint8_t address) { return this->AdapterSensorRawValue[address]; }

int64_t LoggerHER::getAdapterSensorCalcValue(uint8_t address) { return this->AdapterSensorCalcValue[address]; }

void LoggerHER::setChipid(int64_t id) { this->chipid = id; }

void LoggerHER::setState(uint8_t in) { this->state = in; }

void LoggerHER::setFileName(char *name)
{
  strcpy(currentFile, name);
  // make sure there is an empty file!
  if (!SD.begin(this->sd_cs, *this->spi, 80000000))
  {
    Serial.println("Card Mount Failed");
    return;
  }

  File myfile = SD.open(currentFile, FILE_WRITE);
  delay(50);
  if (myfile.available() > 0)
  {
    Serial.print("Size =");
    Serial.println(myfile.size());
    Serial.print(currentFile);
    Serial.println(" is not empty"); // Check if File is empty
    myfile.close();
    SD.end();
  }
  else
  {
    Serial.print(currentFile);
    Serial.println(" is empty");
    myfile.print(""); // add line
    myfile.close();
    SD.end();
  }
}

void LoggerHER::writeHeader()
{
  if (!SD.begin(this->sd_cs, *this->spi, 80000000))
  {
    Serial.println("Card Mount Failed");
    return;
  }

  File myfile = SD.open(currentFile, FILE_WRITE);
  String toSD = "Time, LoggerID, DiveID,";
  int i       = 1;
  delay(50);
  myfile.print(toSD);
  while (i <= this->AdapterNum_Sensors)
  {
    toSD = "Sensor_" + String(i) + "_Sensor_ID, Sensor_" + String(i) + "_RawValue,";
    myfile.print(toSD);
    i++;
  }
  myfile.println();
  myfile.close();
  SD.end();
}

void LoggerHER::setID(uint8_t id) { this->id = id; }

void LoggerHER::setDiveId(uint8_t dive) { this->diveID = dive; }

void LoggerHER::setScheduler_BootCount(uint32_t Bootcount) { this->Scheduler_Bootcount = Bootcount; }

int64_t LoggerHER::getChipid() { return this->chipid; }

uint8_t LoggerHER::getState() { return this->state; }

uint8_t LoggerHER::getID() { return this->id; }

uint8_t LoggerHER::getDiveId() { return this->diveID; }

uint32_t LoggerHER::getScheduler_BootCount() { return this->Scheduler_Bootcount; }

const char *LoggerHER::getFileName() { return this->currentFile; }