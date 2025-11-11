/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: I2C Slave Interface
 */

#ifndef I2C_SLAVE_H_
#define I2C_SLAVE_H_

// Commands list
enum CMD_LIST
{
  CMD_GETVER = 0x00, // master could enumerate Slaves with get version
  CMD_UPDATE = 0x01, // Master wants to send a new firmware -> Slave must write own flash! TODO!
  CMD_GETVALUE1 = 0x03,
  CMD_GETVALUE2 = 0x04,
  CMD_CONVERT = 0x05,
  CMD_SENSOR_SLEEP = 0x06,
  CMD_SENSOR_WAKEUP = 0x07,
  CMD_SET_TEMP = 0x08, // Master sends last known temperature of the water (3 byte command)
  CMD_GET_SENSORVOLTAGE = 0x09,
  CMD_GETRAWVALUE1 = 0x13,
  CMD_GETRAWVALUE2 = 0x14,
  CMD_GET_PARAMETER = 0x15,
  CMD_GET_RDY = 0x16,
  CMD_SET_CALIB = 0x17, // 6 bytes: cmd, #num, 4byte floating point
  CMD_GET_CALIBRATED = 0x18,
  CMD_GET_SENSOR_WAKEUP_TIME = 0x19,
  CMD_GET_FW_VERSION = 0x20,
  CMD_SOFTWARE_RESET = 0x21,
  CMD_GET_EXTERNPARAMETER = 0x22,
  CMD_PING = 0xAA, // master wants a answer byte (seems to be unnecessary, because of getver)

  CMD_1ByteDummyTest = 0xFE,

  CMD_UNKNOWN = 0xFF,
};

// Responds list
enum RES_LIST
{
  RES_OK = 0x01,
  RES_PONG = 0x55,
  RES_ERROR = 0xFF,
};

// Parameter list
enum PAR_LIST
{
  PAR_OFF = 0x01, // switch sensor on
  PAR_ON = 0x02,  // switch sensor off
  PAR_UNKNOWN = 0xFF,
};

void I2C_slaveInit(void (*SCallback)(),
                   void (*TCallback)(unsigned char volatile *value),
                   void (*RCallback)(unsigned char value),
                   unsigned char slave_address);

#endif /* I2CSLAVE_I2C_SLAVE_H_ */
