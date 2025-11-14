/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Implementation of SMBus (System Management Bus) communication functions
 */

#ifndef SMBus_H_
#define SMBus_H_

enum Datatypes
{
  UInt_1,
  UInt_2,
  UInt_3,
  UInt_4,
  Int_1,
  Int_2,
  Int_3,
  Int_4,
  Float_min
};

class SMBus
{
public:
  SMBus();
  void Init(TwoWire &wirePort, uint8_t *blockBuffer, uint8_t blockBufferLen, uint8_t BMS_address);
  void send_CMD(uint8_t Command);
  void write_Word(uint8_t Command, uint16_t Data);
  void write_dWord_BE(uint8_t Command, uint32_t Data);
  void write_Block(uint16_t Data);
  void bmsUndervoltageProtection();
  void bmsShutdown();
  uint8_t read_Word(uint8_t Register, uint8_t length);
  uint8_t read_Block(uint8_t length);

private:
  TwoWire *_i2cPort;
  uint8_t BMS_address;

  uint8_t MBA_Command = 0x44;
  uint8_t blockBufferLen = 8;
  uint8_t *i2c_Buffer;
  /*
          uint16_t FirmwareVersion;
          uint16_t HarwareVersion;
          uint16_t Instr_Flash_Signature;
          uint16_t Static_DF_Signature;
          uint16_t Chemical_ID;
          uint8_t Static_Chem_DF_Signature;
          uint8_t All_DF_Signature;
          uint8_t ShutdownMode;
          uint8_t SleepMode;
          uint8_t AutoCCOfset;
          uint8_t PreDischargeFETToggle;
          uint8_t FuseToggle;
          uint8_t PrechargeFETToggle;
          uint8_t ChargeFETToggle;
          uint8_t DischargeFETToggle;
          uint8_t Gauging;
          uint8_t FETControl;
          uint8_t LifetimeDataCollection;
          uint8_t PermanentFailure;
          uint8_t BlackBoxRecorder;
          uint8_t Fuse;
          uint8_t LEDDisplayEnable;
          uint8_t LifetimeDataReset;
          uint8_t PermanentFailureData_Reset;
          uint8_t BlackBoxRecorder_Reset;
          uint8_t LEDToggle;
          uint8_t LEDDisplayPress;
          uint8_t CalibrationMode;
          uint8_t LifetimeDataFlush;
          uint8_t LifetimeDataSpeedUp_Mode;
          uint8_t SealDevice;
          uint8_t SecurityKeys;
          uint8_t AuthenticationKey;
          uint8_t DeviceReset;
          uint8_t SafetyAlert;
          uint8_t SafetyStatus;
          uint8_t PFAlert;
          uint8_t PFStatus;
          uint8_t OperationStatus;
          uint8_t ChargingStatus;
          uint8_t GaugingStatus;
          uint8_t ManufacturingStatus;
          uint8_t AFERegister;
          uint8_t NoLoadRemCap;
          uint8_t LifetimeDataBlock1;
          uint8_t LifetimeDataBlock2;
          uint8_t LifetimeDataBlock3;
          uint8_t LifetimeDataBlock4;
          uint8_t LifetimeDataBlock5;
          uint8_t ManufacturerInfo;
          uint8_t DAStatus1;
          uint8_t DAStatus2;
          uint8_t GaugingStatus1;
          uint8_t GaugingStatus2;
          uint8_t GaugingStatus3;
          uint8_t CBStatus;
          uint8_t StateofHealth;
          uint8_t FilterCapacity;
          uint8_t RSOC_Write;
          uint8_t ManufacturerInfoB;
          uint8_t DAStatus3;
          uint8_t GaugingStatus4;
          uint8_t GaugingStatus5;
          uint8_t ManufacturerInfoC;
          uint8_t ManufacturerInfoD;
          uint8_t CurrentLong;
          uint8_t IATA_SHUTDOWN;
          uint8_t IATA_RM;
          uint8_t IATA_FCC;
          uint8_t IATA_Charge;
          uint8_t ROMMode;
          uint8_t ExitCalibrationOutput;
          uint8_t Output_CCADC_Cal;
          uint8_t OutputShortedCCADCCal;
          uint8_t Output_Cell_Seven_CCADC_cal;
          */
};

#endif