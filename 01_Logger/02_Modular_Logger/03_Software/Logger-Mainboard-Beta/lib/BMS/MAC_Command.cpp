/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Command definitions and implementations
 */

int MBA_Command = 0x44; // Manufacturer Block Access
enum MAC_CMD_List
{
  CMD_ManufacturerAccess          = 0x0000, // master could enumerate Slaves with get version
  CMD_Device_Type                 = 0x0001,
  CMD_FirmwareVersion             = 0x0002,
  CMD_HarwareVersion              = 0x0003,
  CMD_Instr_Flash_Signature       = 0x0004,
  CMD_Static_DF_Signature         = 0x0005,
  CMD_Chemical_ID                 = 0x0006,
  CMD_Static_Chem_DF_Signature    = 0x0008,
  CMD_All_DF_Signature            = 0x0009,
  CMD_ShutdownMode                = 0x0010,
  CMD_SleepMode                   = 0x0011,
  CMD_AutoCCOfset                 = 0x0013,
  CMD_PreDischargeFETToggle       = 0x001C,
  CMD_FuseToggle                  = 0x001D,
  CMD_PrechargeFETToggle          = 0x001E,
  CMD_ChargeFETToggle             = 0x001F,
  CMD_DischargeFETToggle          = 0x0020,
  CMD_Gauging                     = 0x0021,
  CMD_FETControl                  = 0x0022,
  CMD_LifetimeDataCollection      = 0x0023,
  CMD_PermanentFailure            = 0x0024,
  CMD_BlackBoxRecorder            = 0x0025,
  CMD_Fuse                        = 0x0026,
  CMD_LEDDisplayEnable            = 0x0027,
  CMD_LifetimeDataReset           = 0x0028,
  CMD_PermanentFailureData_Reset  = 0x0029,
  CMD_BlackBoxRecorder_Reset      = 0x002A,
  CMD_LEDToggle                   = 0x002B,
  CMD_LEDDisplayPress             = 0x002C,
  CMD_CalibrationMode             = 0x002D,
  CMD_LifetimeDataFlush           = 0x002E,
  CMD_LifetimeDataSpeedUp_Mode    = 0x002F,
  CMD_SealDevice                  = 0x0030,
  CMD_SecurityKeys                = 0x0035,
  CMD_AuthenticationKey           = 0x0037,
  CMD_DeviceReset                 = 0x0041,
  CMD_SafetyAlert                 = 0x0050,
  CMD_SafetyStatus                = 0x0051,
  CMD_PFAlert                     = 0x0052,
  CMD_PFStatus                    = 0x0053,
  CMD_OperationStatus             = 0x54,
  CMD_ChargingStatus              = 0x0055,
  CMD_GaugingStatus               = 0x0056,
  CMD_ManufacturingStatus         = 0x0057,
  CMD_AFERegister                 = 0x0058,
  CMD_NoLoadRemCap                = 0x005A,
  CMD_LifetimeDataBlock1          = 0x0060,
  CMD_LifetimeDataBlock2          = 0x0061,
  CMD_LifetimeDataBlock3          = 0x0062,
  CMD_LifetimeDataBlock4          = 0x0063,
  CMD_LifetimeDataBlock5          = 0x0064,
  CMD_ManufacturerInfo            = 0x0070,
  CMD_DAStatus1                   = 0x0071,
  CMD_DAStatus2                   = 0x0072,
  CMD_GaugingStatus1              = 0x0073,
  CMD_GaugingStatus2              = 0x0074,
  CMD_GaugingStatus3              = 0x0075,
  CMD_CBStatus                    = 0x0076,
  CMD_StateofHealth               = 0x0077,
  CMD_FilterCapacity              = 0x0078,
  CMD_RSOC_Write                  = 0x0079,
  CMD_ManufacturerInfoB           = 0x007A,
  CMD_DAStatus3                   = 0x007B,
  CMD_GaugingStatus4              = 0x007C,
  CMD_GaugingStatus5              = 0x007D,
  CMD_ManufacturerInfoC           = 0x0080,
  CMD_ManufacturerInfoD           = 0x0081,
  CMD_CurrentLong                 = 0x0082,
  CMD_IATA_SHUTDOWN               = 0x00F0,
  CMD_IATA_RM                     = 0x00F1,
  CMD_IATA_FCC                    = 0x00F2,
  CMD_IATA_Charge                 = 0x00F3,
  CMD_ROMMode                     = 0x0F00,
  CMD_ExitCalibrationOutput       = 0xF080,
  CMD_Output_CCADC_Cal            = 0xF081,
  CMD_OutputShortedCCADCCal       = 0xF082,
  CMD_Output_Cell_Seven_CCADC_cal = 0xF083,
  CMD_FET_EN                      = 0x44,
  CMD_RESET                       = 0x44,
  CMD_ITStatus                    = 0x0056
};

enum BMS_REGISTER_LIST
{
  Register_Voltage           = 0x9, // Sum of all Cellvoltages
  Register_Current           = 0xA,
  Register_AvgCurrent        = 0xB, // Average Current
  Register_RSOC              = 0xD, // Relative State of Charge
  Register_ASOC              = 0xE, // Abspöite State of Charge
  Register_BatteryStatus     = 0x16,
  DesignVoltage              = 0x19,
  Register_BatteryMode       = 0x03,
  Register_TempRange         = 0x55,
  Register_RemainingCapacity = 0xf,
  Register_Temperature       = 0x8,
  Register_IntTemp           = 0x72,
  Register_TS1Temp           = 0x72,
  Register_TS2Temp           = 0x72,
  Register_Cell_V            = 0x71,
  Register_Cell_I            = 0x71,
  Register_BATpinVoltage     = 0x71,
  Register_PACKpinVoltage    = 0x71,
  Register_StateofHealth     = 0x4f,
  Register_SerialNumber      = 0x1C
};