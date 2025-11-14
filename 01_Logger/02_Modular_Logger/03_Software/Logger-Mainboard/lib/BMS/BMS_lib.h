/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: BMS (Battery Management System) library functions
 */

#ifndef BMS_LIB_H
#define BMS_LIB_H
#include "SMBus.h"
// #include "SBS_Structs.h"
enum BMSState
{
    BMS_Discharging,
    BMS_Charging,
    BMS_Undefined_State
};

class BMS_IC
{
public:
    BMS_IC();
    void Init(TwoWire &wirePort, uint8_t *blockBuffer, uint8_t blockBufferLen, uint8_t BMS_address);
    uint16_t getDesignVoltage();
    uint8_t getcheckFETStatus();
    uint16_t setDesignVoltage();
    uint32_t setFET_EN();
    uint32_t setRESET();
    uint16_t getBatteryMode();
    uint16_t getOperationStatusA();
    uint16_t getOperationStatusB();
    uint8_t getTempRange();
    uint16_t getITStatus();
    uint16_t getLStatus();
    uint16_t getSafetyAlertAB();
    uint16_t getSafetyAlertCD();
    uint16_t getSafetyStatusAB();
    uint16_t getSafetyStatusCD();
    uint16_t getPFAlertAB();
    uint16_t getPFAlertCD();
    uint16_t getPFStatusAB();
    uint16_t getPFStatusCD();
    uint16_t getRemainingCapacity();
    uint16_t getTemperature();
    uint16_t getIntTemp();
    uint16_t getTS1Temp();
    uint16_t getTS2Temp();
    uint16_t getCell1_V();
    uint16_t getCell2_V();
    uint16_t getCell3_V();
    uint16_t getCell4_V();
    int16_t getCell1_I();
    int16_t getCell2_I();
    int16_t getCell3_I();
    int16_t getCell4_I();
    uint16_t getBATpinVoltage();
    uint16_t getPACKpinVoltage();
    uint16_t getStateofHealth();
    uint16_t getChargingStatus();
    uint8_t getGaugingStatus();
    void setBmsUndervoltageProtection();
    void setBmsShutdown();
    uint16_t getSerialNumber();
    /* uint16_t getFirmwareVersion();
     uint16_t getHardwareVersion();
     uint16_t getInstr_Flash_Signature();
     uint16_t getStatic_DF_Signature();
     uint16_t getChemicalID();*/
    uint16_t getSumCellVoltage();
    int16_t getCurrent();
    int16_t getAvgCurrent();
    uint8_t getRSOC();
    uint8_t getASOC();
    uint16_t getBatteryStatus();
    uint16_t getChemicalID();
    uint32_t getOperationStatus();
    // uint32_t getChargingStatus();
    // uint32_t getGaugingStatus();
    uint16_t getManufacturingStatus();
    uint16_t getStateOfHealth_FCC();
    uint16_t getStateOfHealth_Energy();
    SMBus _SMBus;
    uint8_t BMS_address;
    uint8_t *i2c_Buffer;

private:
    // Register Zugriff
    uint16_t CellSumVoltage;
    int16_t Current;
    int16_t AvgCurrent;
    uint8_t RSOC;
    uint8_t ASOC;
    uint16_t BatteryStatus;

    // MAC Zugriff
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
    uint32_t OperationStatus;
    uint32_t ChargingStatus;
    uint32_t GaugingStatus;
    uint16_t ManufacturingStatus;
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
    uint16_t StateofHealth_FCC;
    uint16_t StateofHealth_Energy;
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

    /*  uint16_t CMD_ManufacturerAccess  = 0x0000; //master could enumerate Slaves with get version
      uint16_t CMD_Device_Type         = 0x0001;
      uint16_t CMD_FirmwareVersion     = 0x0002;
      uint16_t CMD_HarwareVersion      = 0x0003;
      uint16_t CMD_Instr_Flash_Signature      = 0x0004;
      uint16_t CMD_Static_DF_Signature        = 0x0005;
      uint16_t CMD_Chemical_ID                = 0x0006;
      uint16_t CMD_Static_Chem_DF_Signature   = 0x0008;
      uint16_t CMD_All_DF_Signature        = 0x0009;
      uint16_t CMD_ShutdownMode            = 0x0010;
      uint16_t CMD_SleepMode               = 0x0011;
      uint16_t CMD_AutoCCOfset             = 0x0013;
      uint16_t CMD_PreDischargeFETToggle   = 0x001C;
      uint16_t CMD_FuseToggle              = 0x001D;
      uint16_t CMD_PrechargeFETToggle      = 0x001E;
      uint16_t CMD_ChargeFETToggle         = 0x001F;
      uint16_t CMD_DischargeFETToggle      = 0x0020;
      uint16_t CMD_Gauging                 = 0x0021;
      uint16_t CMD_FETControl              = 0x0022;
      uint16_t CMD_LifetimeDataCollection  = 0x0023;
      uint16_t CMD_PermanentFailure        = 0x0024;
      uint16_t CMD_BlackBoxRecorder        = 0x0025;
      uint16_t CMD_Fuse                    = 0x0026;
      uint16_t CMD_LEDDisplayEnable        = 0x0027;
      uint16_t CMD_LifetimeDataReset       = 0x0028;
      uint16_t CMD_PermanentFailureData_Reset  = 0x0029;
      uint16_t CMD_BlackBoxRecorder_Reset      = 0x002A;
      uint16_t CMD_LEDToggle                   = 0x002B;
      uint16_t CMD_LEDDisplayPress             = 0x002C;
      uint16_t CMD_CalibrationMode             = 0x002D;
      uint16_t CMD_LifetimeDataFlush           = 0x002E;
      uint16_t CMD_LifetimeDataSpeedUp_Mode    = 0x002F;
      uint16_t CMD_SealDevice          = 0x0030;
      uint16_t CMD_SecurityKeys        = 0x0035;
      uint16_t CMD_AuthenticationKey   = 0x0037;
      uint16_t CMD_DeviceReset         = 0x0041;
      uint16_t CMD_SafetyAlert         = 0x0050;
      uint16_t CMD_SafetyStatus        = 0x0051;
      uint16_t CMD_PFAlert             = 0x0052;
      uint16_t CMD_PFStatus            = 0x0053;
      uint16_t CMD_OperationStatus     = 0x54;
      uint16_t CMD_ChargingStatus      = 0x0055;
      uint16_t CMD_GaugingStatus       = 0x0056;
      uint16_t CMD_ManufacturingStatus = 0x0057;
      uint16_t CMD_AFERegister         = 0x0058;
      uint16_t CMD_NoLoadRemCap        = 0x005A;
      uint16_t CMD_LifetimeDataBlock1  = 0x0060;
      uint16_t CMD_LifetimeDataBlock2  = 0x0061;
      uint16_t CMD_LifetimeDataBlock3  = 0x0062;
      uint16_t CMD_LifetimeDataBlock4  = 0x0063;
      uint16_t CMD_LifetimeDataBlock5  = 0x0064;
      uint16_t CMD_ManufacturerInfo    = 0x0070;
      uint16_t CMD_DAStatus1           = 0x0071;
      uint16_t CMD_DAStatus2           = 0x0072;
      uint16_t CMD_GaugingStatus1      = 0x0073;
      uint16_t CMD_GaugingStatus2      = 0x0074;
      uint16_t CMD_GaugingStatus3      = 0x0075;
      uint16_t CMD_CBStatus            = 0x0076;
      uint16_t CMD_StateofHealth       = 0x0077;
      uint16_t CMD_FilterCapacity      = 0x0078;
      uint16_t CMD_RSOC_Write          = 0x0079;
      uint16_t CMD_ManufacturerInfoB   = 0x007A;
      uint16_t CMD_DAStatus3           = 0x007B;
      uint16_t CMD_GaugingStatus4      = 0x007C;
      uint16_t CMD_GaugingStatus5      = 0x007D;
      uint16_t CMD_ManufacturerInfoC   = 0x0080;
      uint16_t CMD_ManufacturerInfoD   = 0x0081;
      uint16_t CMD_CurrentLong         = 0x0082;
      uint16_t CMD_IATA_SHUTDOWN       = 0x00F0;
      uint16_t CMD_IATA_RM             = 0x00F1;
      uint16_t CMD_IATA_FCC            = 0x00F2;
      uint16_t CMD_IATA_Charge         = 0x00F3;
      uint16_t CMD_ROMMode             = 0x0F00;
      uint16_t CMD_ExitCalibrationOutput       = 0xF080;
      uint16_t CMD_Output_CCADC_Cal            = 0xF081;
      uint16_t CMD_OutputShortedCCADCCal       = 0xF082;
      uint16_t CMD_Output_Cell_Seven_CCADC_cal = 0xF083;*/
};

#endif