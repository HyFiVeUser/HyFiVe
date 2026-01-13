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
#include "../../src/LedManager.h"
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
		generalError();
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

void SMBus::bmsShutdown()
{
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(0x44);
	_i2cPort->write(0x02);
	_i2cPort->write(0x10);
	_i2cPort->write(0x00);
	_i2cPort->write(0x3B);
	_i2cPort->endTransmission();
}

void SMBus::bmsUndervoltageProtection()
{
	// "Protections","CUV","Threshold","2500","mV"
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

	//"Power","Shutdown","Shutdown Voltage","2750","mV"
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
	_i2cPort->write(0x64); //! 100 0x64
	_i2cPort->write(0x64); //! 100 0x64
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

	// DF write: start at 0x4AF8, write 2 bytes (0x000A = 10 mA)
	_i2cPort->beginTransmission(BMS_address);
	_i2cPort->write(0x44); // ManufacturerBlockAccess
	_i2cPort->write(0x04); // Länge: 2 (Adresse) + 2 (Daten)
	_i2cPort->write(0xF8); // Startadresse low  (0x4AF8)
	_i2cPort->write(0x4A); // Startadresse high
	_i2cPort->write(0x0A); // Sleep Current LSB = 10
	_i2cPort->write(0x00); // MSB
	_i2cPort->endTransmission();

	delay(100);

	// CUVC off
	// "Settings","Protection","Enabled Protections B","3a","hex"
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
	_i2cPort->write(0x64); //! 100 0x64
	_i2cPort->write(0x00);
	_i2cPort->write(0x14);
	_i2cPort->write(0x64); //! 100 0x64
	_i2cPort->write(0x64); //! 100 0x64
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

	delay(100);

	// OCVFR off
	//"Settings","Configuration","IT Gauging Configuration","d1c6","hex"
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(0x44);
	_i2cPort->write(0x22);
	_i2cPort->write(0x80);
	_i2cPort->write(0x4B);
	_i2cPort->write(0x80);
	_i2cPort->write(0x0C);
	_i2cPort->write(0xE4);
	_i2cPort->write(0x0C);
	_i2cPort->write(0x06);
	_i2cPort->write(0x08);
	_i2cPort->write(0x68);
	_i2cPort->write(0x10);
	_i2cPort->write(0x36);
	_i2cPort->write(0x10);
	_i2cPort->write(0x64); //! 100 0x64
	_i2cPort->write(0x5F);
	_i2cPort->write(0x01);
	_i2cPort->write(0x6F);
	_i2cPort->write(0x01);
	_i2cPort->write(0x02);
	_i2cPort->write(0x02);
	_i2cPort->write(0x02);
	_i2cPort->write(0x02);
	_i2cPort->write(0x03);
	_i2cPort->write(0x50);
	_i2cPort->write(0x46);
	_i2cPort->write(0x00);
	_i2cPort->write(0x00);
	_i2cPort->write(0x50);
	_i2cPort->write(0x5F);
	_i2cPort->write(0x3C);
	_i2cPort->write(0x64); //! 100 0x64
	_i2cPort->write(0x00);
	_i2cPort->write(0xC6);
	_i2cPort->write(0xD1);
	_i2cPort->write(0xFF);
	_i2cPort->write(0x96);
	_i2cPort->endTransmission();

	delay(100);

	// Settings -> Configuration -> SBS Gauging Configuration: 0x4AC9 (H1) von 0x04 auf 0x05
	//    (bit0 RSOCL gesetzt). Adresse/Typ: H1 @ 0x4AC9.  Count = 2+1 = 0x03
	//    Ref: Data Flash Table (SBS Gauging Configuration).
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(0x44); // ManufacturerBlockAccess
	_i2cPort->write(0x03); // Count = 2 (Addr) + 1 (Data)
	_i2cPort->write(0xC9); // Addr L = 0xC9
	_i2cPort->write(0x4A); // Addr H = 0x4A
	_i2cPort->write(0x05); // Wert = 0x05
	_i2cPort->endTransmission();
	delay(100);

	// Protections -> COV -> Threshold Low Temp:        0x4BCC (I2) von 4300 auf 4250 mV => 0x109A -> 0x9A 0x10
	//    Count = 2+2 = 0x04
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(0x44);
	_i2cPort->write(0x04);
	_i2cPort->write(0xCC);
	_i2cPort->write(0x4B);
	_i2cPort->write(0x9A); // 4250 mV L
	_i2cPort->write(0x10); // 4250 mV H
	_i2cPort->endTransmission();
	delay(100);

	// Protections -> COV -> Threshold Standard Temp Low: 0x4BCE (I2) -> 4250 mV
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(0x44);
	_i2cPort->write(0x04);
	_i2cPort->write(0xCE);
	_i2cPort->write(0x4B);
	_i2cPort->write(0x9A);
	_i2cPort->write(0x10);
	_i2cPort->endTransmission();
	delay(100);

	// Protections -> COV -> Threshold Standard Temp High: 0x4BD0 (I2) -> 4250 mV
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(0x44);
	_i2cPort->write(0x04);
	_i2cPort->write(0xD0);
	_i2cPort->write(0x4B);
	_i2cPort->write(0x9A);
	_i2cPort->write(0x10);
	_i2cPort->endTransmission();
	delay(100);

	// Protections -> COV -> Threshold High Temp:        0x4BD2 (I2) -> 4250 mV
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(0x44);
	_i2cPort->write(0x04);
	_i2cPort->write(0xD2);
	_i2cPort->write(0x4B);
	_i2cPort->write(0x9A);
	_i2cPort->write(0x10);
	_i2cPort->endTransmission();
	delay(100);

	// Protections -> COV -> Threshold Rec Temp:         0x4BD4 (I2) -> 4250 mV
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(0x44);
	_i2cPort->write(0x04);
	_i2cPort->write(0xD4);
	_i2cPort->write(0x4B);
	_i2cPort->write(0x9A);
	_i2cPort->write(0x10);
	_i2cPort->endTransmission();
	delay(100);

	// Advanced Charge Algorithm -> Termination Config -> Charge Term Taper Current:
	//    Adresse 0x4CF3 (I2), von 550 auf 100 mA => 0x0064 -> 0x64 0x00
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(0x44);
	_i2cPort->write(0x04);
	_i2cPort->write(0xF3);
	_i2cPort->write(0x4C);
	_i2cPort->write(0x64); // 100 mA L
	_i2cPort->write(0x00); // 100 mA H
	_i2cPort->endTransmission();
	delay(100);

	// Advanced Charge Algorithm -> Termination Config -> Charge Term Voltage:
	//    Adresse 0x4CF7 (I2), von 75 auf 10 mV => 0x000A -> 0x0A 0x00
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(0x44);
	_i2cPort->write(0x04);
	_i2cPort->write(0xF7);
	_i2cPort->write(0x4C);
	_i2cPort->write(0x0A); // 10 mV L
	_i2cPort->write(0x00); // 10 mV H
	_i2cPort->endTransmission();
	delay(100);

	// SBS Configuration -> Data -> Serial Number: 0x40CC (H2) von 0x0002 auf 0x0003
	//    Achtung: H2 ist 2-Byte Hex, trotzdem Little-Endian beim Schreiben.
	_i2cPort->beginTransmission(this->BMS_address);
	_i2cPort->write(0x44);
	_i2cPort->write(0x04); // Count = 2 + 2
	_i2cPort->write(0xCC); // Addr L = 0xCC
	_i2cPort->write(0x40); // Addr H = 0x40
	_i2cPort->write(0x03); // 0x0003 L
	_i2cPort->write(0x00); // 0x0003 H
	_i2cPort->endTransmission();
	delay(100);
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
