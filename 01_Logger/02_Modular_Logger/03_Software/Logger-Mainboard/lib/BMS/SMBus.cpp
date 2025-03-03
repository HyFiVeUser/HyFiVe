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

#include <Wire.h>
#include "SMBus.h"
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "../../src/Led.h"
// #include "MAC_Command.cpp"

SMBus::SMBus()
{
}

void logError(const char *message)
{
	File logFile = SD.open("/log/log.txt", FILE_APPEND);
	if (logFile)
	{
		logFile.println(message);
		logFile.close();
	}
}

void SMBus::Init(TwoWire &wirePort, uint8_t *blockBuffer, uint8_t blockBufferLen, uint8_t BMS_address)
{ //(TwoWire &wirePort, uint8_t* blockBuffer, uint8_t blockBufferLen, uint8_t BMS_address){//*/){
	this->_i2cPort = &wirePort;
	this->i2c_Buffer = blockBuffer;
	this->blockBufferLen = blockBufferLen;
	this->BMS_address = BMS_address;
	// _i2cPort->begin(21, 14, (uint32_t)100000); // Frequenz ggf. runtersetzen --> Crosscheck mit EVM

	if (!_i2cPort->begin(21, 14, (uint32_t)100000))
	{
		logError("[BMS];[ERROR];;;BMS not found");
		generalAlarmLed();
		// ESP.restart();
	}
}

void SMBus::send_CMD(uint8_t Command)
{
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(Command);
	_i2cPort->endTransmission();
}

void SMBus::write_Word(uint8_t Command, uint16_t Data)
{
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(Command);
	_i2cPort->write(uint8_t(Data & 0xFF)); // Low Byte First
	_i2cPort->write(uint8_t(Data >> 8));   // High Byte Second
	_i2cPort->endTransmission();
}

void SMBus::undervoltageProtection()
{
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(0x44);
	_i2cPort->write(0x22);
	_i2cPort->write(0xC0);
	_i2cPort->write(0x4B);
	_i2cPort->write(0x00);
	_i2cPort->write(0x0C);
	_i2cPort->write(0xC4); // 2500
	_i2cPort->write(0x09); 
	_i2cPort->write(0x02);
	_i2cPort->write(0xB8);
	_i2cPort->write(0x0B);
	_i2cPort->write(0xC4); // 2500
	_i2cPort->write(0x09); 
	_i2cPort->write(0x02);
	_i2cPort->write(0xB8);
	_i2cPort->write(0x0B);
	_i2cPort->write(0xCC);
	_i2cPort->write(0x10);
	_i2cPort->write(0xCC);
	_i2cPort->write(0x10);
	_i2cPort->write(0xCC);
	_i2cPort->write(0x10);
	_i2cPort->write(0xCC);
	_i2cPort->write(0x10);
	_i2cPort->write(0xCC);
	_i2cPort->write(0x10);
	_i2cPort->write(0x02);
	_i2cPort->write(0x3C);
	_i2cPort->write(0x0F);
	_i2cPort->write(0x3C);
	_i2cPort->write(0x0F);
	_i2cPort->write(0x3C);
	_i2cPort->write(0x0F);
	_i2cPort->write(0x3C);
	_i2cPort->write(0x0F);
	_i2cPort->write(0x3C);
	_i2cPort->write(0x08);
	_i2cPort->endTransmission();

	delay(100);

	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(0x44);
	_i2cPort->write(0x22);
	_i2cPort->write(0xE0);
	_i2cPort->write(0x4A);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0x16);
	_i2cPort->write(0xEA);
	_i2cPort->write(0x00);
	_i2cPort->write(0xAC);
	_i2cPort->write(0x0D);
	_i2cPort->write(0xBE); // 2750
	_i2cPort->write(0x0A); // 2750
	_i2cPort->write(0x0A);
	_i2cPort->write(0xD6);
	_i2cPort->write(0x06);
	_i2cPort->write(0x0A);
	_i2cPort->write(0xC4);
	_i2cPort->write(0x09);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0xB8);
	_i2cPort->write(0x0B);
	_i2cPort->write(0x64);
	_i2cPort->write(0x64);
	_i2cPort->write(0x00);
	_i2cPort->write(0x05);
	_i2cPort->write(0xDE);
	_i2cPort->write(0x0A);
	_i2cPort->write(0x6E);
	_i2cPort->write(0x0C);
	_i2cPort->write(0x05);
	_i2cPort->write(0xC3);
	_i2cPort->endTransmission();

	delay(100);

	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(0x44);
	_i2cPort->write(0x22);
	_i2cPort->write(0xA0);
	_i2cPort->write(0x4B);
	_i2cPort->write(0x01);
	_i2cPort->write(0x1A);
	_i2cPort->write(0x00);
	_i2cPort->write(0x07);
	_i2cPort->write(0x03);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0xE8);
	_i2cPort->write(0x03);
	_i2cPort->write(0x96);
	_i2cPort->write(0x00);
	_i2cPort->write(0x64);
	_i2cPort->write(0x00);
	_i2cPort->write(0x14);
	_i2cPort->write(0x64);
	_i2cPort->write(0x64);
	_i2cPort->write(0x01);
	_i2cPort->write(0x03);
	_i2cPort->write(0x0C);
	_i2cPort->write(0x05);
	_i2cPort->write(0xD0);
	_i2cPort->write(0x07);
	_i2cPort->write(0x00);
	_i2cPort->write(0x3F);
	_i2cPort->write(0x3A);
	_i2cPort->write(0xED);
	_i2cPort->endTransmission();
}

void SMBus::write_dWord_BE(uint8_t Command, uint32_t Data)
{ // Big-Endian-Reihenfolge
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(Command);
	_i2cPort->write(uint8_t((Data >> 24) & 0xFF)); // Höherwertiges Byte (High Byte)
	_i2cPort->write(uint8_t((Data >> 16) & 0xFF)); // Drittes Byte
	_i2cPort->write(uint8_t((Data >> 8) & 0xFF));  // Zweites Byte
	_i2cPort->write(uint8_t(Data & 0xFF));		   // Niederwertiges Byte (Low Byte)
	_i2cPort->endTransmission();
}

void SMBus::write_Block(uint16_t Data)
{
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(this->MBA_Command);
	_i2cPort->write(0x2);				   // Grund unbekannt
	_i2cPort->write(uint8_t(Data & 0xFF)); // Low Byte First
	_i2cPort->write(uint8_t(Data >> 8));   // High Byte Second
	_i2cPort->endTransmission();
}
uint8_t SMBus::read_Word(uint8_t Register, uint8_t length)
{
	uint8_t numBytes, i;
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(Register);
	_i2cPort->endTransmission();

	numBytes = _i2cPort->requestFrom(this->BMS_address, length);
	for (i = 0; i < numBytes; i++)
	{
		this->i2c_Buffer[i] = _i2cPort->read();
	}
	return numBytes;
}

uint8_t SMBus::read_Block(uint8_t length)
{
	uint8_t numBytes, i, reallength;
	reallength = length + 4;
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(this->MBA_Command);
	_i2cPort->endTransmission();

	numBytes = _i2cPort->requestFrom(this->BMS_address, reallength);
	for (i = 0; i < numBytes; i++)
	{
		this->i2c_Buffer[i] = _i2cPort->read();
	}
	return numBytes;
}
