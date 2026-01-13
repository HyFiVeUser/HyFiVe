/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Device command definitions and implementations for I2C communication
 */

// #include <stdio.h>

// Commands list
enum CMD_LIST
{
  CMD_GETVER                 = 0x00, // master could enumerate Slaves with get version
  CMD_UPDATE                 = 0x01, // Master wants to send a new firmware -> Slave must write own flash! TODO!
  CMD_SETSENSOR              = 0x02, // Master sets sensor type and i2c slave address (2 byte command)
  CMD_GETVALUE1              = 0x03,
  CMD_GETVALUE2              = 0x04,
  CMD_CONVERT                = 0x05,
  CMD_SENSOR_SLEEP           = 0x06,
  CMD_SENSOR_WAKEUP          = 0x07,
  CMD_SET_TEMP               = 0x08, // Master sends last known temperature of the water (3 byte command)
  CMD_GET_SENSORVOLTAGE      = 0x09,
  CMD_GETRAWVALUE1           = 0x13,
  CMD_GETRAWVALUE2           = 0x14,
  CMD_GET_PARAMETER          = 0x15,
  CMD_GET_RDY                = 0x16,
  CMD_SET_CALIB              = 0x17, // 6 bytes: cmd, #num, 4byte floating point
  CMD_GET_CALIBRATED         = 0x18,
  CMD_GET_SENSOR_WAKEUP_TIME = 0x19,
  CMD_GET_FW_VERSION         = 0x20,
  CMD_SOFTWARE_RESET         = 0x21,
  CMD_PING                   = 0xAA, // master wants a answer byte (seems to be unnecessary, because of getver)

  CMD_1ByteDummyTest = 0xFE,

  CMD_UNKNOWN = 0xFF,
};

//* CMD_GETVER
// ID | Manufacturer        | Parameter         | Model                 | Long name                                 | Unit
//----:---------------------:-------------------:-----------------------:-------------------------------------------:--------------
// 1  : blue_robotics       : pressure          : bar30                 : sea_water_pressure                        : mbar
// 2  : blue_robotics       : temperature       : celsius_fast_response : sea_water_temperature                     : degree_C
// 4  : presens             : oxygen            : NAU-OIW               : mass_concentration_of_oxygen_in_sea_water : mbar
// 5  : presens             : oxygen            : NAU-YOP               : mass_concentration_of_oxygen_in_sea_water : mbar
// 6  : keller              : pressure          : series_20             : sea_water_pressure                        : mbar
// 7  : aml                 : temperature       : ct_xchange            : sea_water_temperature                     : degree_C
// 8  : aml                 : conductivity      : ct_xchange            : sea_water_electrical_conductivity         : mS_cm-1
// 3  : atlas_scientific    : conductivity      : k0.1                  : sea_water_electrical_conductivity         : mS_cm-1
// 10 : atlas_scientific    : conductivity      : k1.0                  : sea_water_electrical_conductivity         : mS_cm-1
// 9  : pyroscience         : oxygen            : oxycap_sub            : partial_pressure_of_oxygen_in_sea_water   : mbar
// 11 : pyroscience         : oxygen            : oxycap_hs_sub         : partial_pressure_of_oxygen_in_sea_water   : mbar
// 12 : Turner              : turbidity         : C-Flour_TRB           : sea_water_turbidity_in_NTU                : NTU

//* CMD_GET_SENSORVOLTAGE
// define information for if sensor needs additional voltages
// #define NEED3V3 1
// #define NEED5V 0
// #define NEED12V 0

//* CMD_GET_PARAMETER
// 0x01 Temperature
// 0x02 Pressure
// 0x03 Oxygen
// 0x04 Conductivity
// 0x05 Turbidity
// 0x06 tbd...
