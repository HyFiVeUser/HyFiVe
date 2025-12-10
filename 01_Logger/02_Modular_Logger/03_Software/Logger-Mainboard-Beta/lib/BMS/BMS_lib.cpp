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


#include <Wire.h>
#include "BMS_lib.h"
#include "SMBus.h"
#include "MAC_Command.cpp"
#include "SBS_Structs.h"

BMS_IC::BMS_IC()
{
}

void BMS_IC::Init(TwoWire &wirePort, uint8_t *blockBuffer, uint8_t blockBufferLen, uint8_t BMS_address)
{
    this->BMS_address = BMS_address;
    this->i2c_Buffer = blockBuffer;
    this->_SMBus.Init(wirePort, blockBuffer, blockBufferLen, BMS_address);
}

uint16_t BMS_IC::getSumCellVoltage()
{
    this->_SMBus.read_Word(Register_Voltage, 2);
    uint8_t byte0 = this->i2c_Buffer[0];
    uint8_t byte1 = this->i2c_Buffer[1];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}

int16_t BMS_IC::getCurrent()
{
    this->_SMBus.read_Word(Register_Current, 2);
    uint8_t byte0 = this->i2c_Buffer[0];
    uint8_t byte1 = this->i2c_Buffer[1];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}

uint8_t BMS_IC::getRSOC()
{
    this->_SMBus.read_Word(Register_RSOC, 1);
    uint8_t value = this->i2c_Buffer[0];
    return value;
}

uint16_t BMS_IC::getRemainingCapacity()
{
    this->_SMBus.read_Word(Register_RemainingCapacity, 2);
    uint8_t byte0 = this->i2c_Buffer[0];
    uint8_t byte1 = this->i2c_Buffer[1];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getTemperature()
{
    this->_SMBus.read_Word(Register_Temperature, 2);
    uint8_t byte0 = this->i2c_Buffer[0];
    uint8_t byte1 = this->i2c_Buffer[1];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getIntTemp()
{
    this->_SMBus.read_Word(Register_IntTemp, 3);
    uint8_t byte0 = this->i2c_Buffer[1];
    uint8_t byte1 = this->i2c_Buffer[2];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getTS1Temp()
{
    this->_SMBus.read_Word(Register_TS1Temp, 5);
    uint8_t byte0 = this->i2c_Buffer[3];
    uint8_t byte1 = this->i2c_Buffer[4];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getTS2Temp()
{
    this->_SMBus.read_Word(Register_TS2Temp, 7);
    uint8_t byte0 = this->i2c_Buffer[5];
    uint8_t byte1 = this->i2c_Buffer[6];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getBATpinVoltage()
{
    this->_SMBus.read_Word(Register_BATpinVoltage, 11);
    uint8_t byte0 = this->i2c_Buffer[9];
    uint8_t byte1 = this->i2c_Buffer[10];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getPACKpinVoltage()
{
    this->_SMBus.read_Word(Register_PACKpinVoltage, 13);
    uint8_t byte0 = this->i2c_Buffer[11];
    uint8_t byte1 = this->i2c_Buffer[12];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getCell1_V()
{
    this->_SMBus.read_Word(Register_Cell_V, 3);
    uint8_t byte0 = this->i2c_Buffer[1];
    uint8_t byte1 = this->i2c_Buffer[2];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}
uint16_t BMS_IC::getCell2_V()
{
    this->_SMBus.read_Word(Register_Cell_V, 5);
    uint8_t byte0 = this->i2c_Buffer[3];
    uint8_t byte1 = this->i2c_Buffer[4];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}
uint16_t BMS_IC::getCell3_V()
{
    this->_SMBus.read_Word(Register_Cell_V, 7);
    uint8_t byte0 = this->i2c_Buffer[5];
    uint8_t byte1 = this->i2c_Buffer[6];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}
uint16_t BMS_IC::getCell4_V()
{
    this->_SMBus.read_Word(Register_Cell_V, 9);
    uint8_t byte0 = this->i2c_Buffer[7];
    uint8_t byte1 = this->i2c_Buffer[8];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}
int16_t BMS_IC::getCell1_I()
{
    this->_SMBus.read_Word(Register_Cell_I, 15);
    uint8_t byte0 = this->i2c_Buffer[13];
    uint8_t byte1 = this->i2c_Buffer[14];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}
int16_t BMS_IC::getCell2_I()
{
    this->_SMBus.read_Word(Register_Cell_I, 17);
    uint8_t byte0 = this->i2c_Buffer[15];
    uint8_t byte1 = this->i2c_Buffer[16];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}
int16_t BMS_IC::getCell3_I()
{
    this->_SMBus.read_Word(Register_Cell_I, 19);
    uint8_t byte0 = this->i2c_Buffer[17];
    uint8_t byte1 = this->i2c_Buffer[18];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}
int16_t BMS_IC::getCell4_I()
{
    this->_SMBus.read_Word(Register_Cell_I, 21);
    uint8_t byte0 = this->i2c_Buffer[19];
    uint8_t byte1 = this->i2c_Buffer[20];
    int16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getStateofHealth()
{
    this->_SMBus.read_Word(Register_StateofHealth, 2);
    uint8_t byte0 = this->i2c_Buffer[0];
    uint8_t byte1 = this->i2c_Buffer[1];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getBatteryStatus()
{ // byte0 Bit 0,1.2 fehlerhaft
    this->_SMBus.read_Word(Register_BatteryStatus, 2);
    uint8_t byte0 = this->i2c_Buffer[0];
    uint8_t byte1 = this->i2c_Buffer[1];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getBatteryMode()
{
    this->_SMBus.read_Word(Register_BatteryMode, 2);
    uint8_t byte0 = this->i2c_Buffer[0];
    uint8_t byte1 = this->i2c_Buffer[1];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}

uint8_t BMS_IC::getTempRange()
{
    this->_SMBus.read_Word(Register_TempRange, 2);
    uint8_t value = this->i2c_Buffer[1];
    return value;
}

uint16_t BMS_IC::getOperationStatusA()
{
    this->_SMBus.read_Word(CMD_OperationStatus, 4);
    uint8_t byte0 = this->i2c_Buffer[1];
    uint8_t byte1 = this->i2c_Buffer[2];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}
uint16_t BMS_IC::getOperationStatusB()
{
    this->_SMBus.read_Word(CMD_OperationStatus, 5);
    uint8_t byte0 = this->i2c_Buffer[3];
    uint8_t byte1 = this->i2c_Buffer[4];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getChargingStatus()
{
    this->_SMBus.read_Word(CMD_ChargingStatus, 4);
    uint8_t byte0 = this->i2c_Buffer[2];
    uint8_t byte1 = this->i2c_Buffer[3];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}

uint8_t BMS_IC::getGaugingStatus()
{
    this->_SMBus.read_Word(CMD_GaugingStatus, 2);
    uint8_t value = this->i2c_Buffer[1];
    return value;
}

uint16_t BMS_IC::getITStatus()
{
    this->_SMBus.read_Word(CMD_ITStatus, 4);
    uint8_t byte0 = this->i2c_Buffer[2];
    uint8_t byte1 = this->i2c_Buffer[3];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getManufacturingStatus()
{
    this->_SMBus.read_Word(CMD_ManufacturingStatus, 3);
    uint8_t byte0 = this->i2c_Buffer[1];
    uint8_t byte1 = this->i2c_Buffer[2];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getSafetyAlertAB()
{
    this->_SMBus.read_Word(CMD_SafetyAlert, 4);
    uint8_t byte0 = this->i2c_Buffer[1];
    uint8_t byte1 = this->i2c_Buffer[2];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}
uint16_t BMS_IC::getSafetyAlertCD()
{
    this->_SMBus.read_Word(CMD_SafetyAlert, 5);
    uint8_t byte0 = this->i2c_Buffer[3];
    uint8_t byte1 = this->i2c_Buffer[4];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getSafetyStatusAB()
{
    this->_SMBus.read_Word(CMD_SafetyStatus, 4);
    uint8_t byte0 = this->i2c_Buffer[1];
    uint8_t byte1 = this->i2c_Buffer[2];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}
uint16_t BMS_IC::getSafetyStatusCD()
{
    this->_SMBus.read_Word(CMD_SafetyStatus, 5);
    uint8_t byte0 = this->i2c_Buffer[3];
    uint8_t byte1 = this->i2c_Buffer[4];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getPFAlertAB()
{
    this->_SMBus.read_Word(CMD_PFAlert, 4);
    uint8_t byte0 = this->i2c_Buffer[1];
    uint8_t byte1 = this->i2c_Buffer[2];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}
uint16_t BMS_IC::getPFAlertCD()
{
    this->_SMBus.read_Word(CMD_PFAlert, 5);
    uint8_t byte0 = this->i2c_Buffer[3];
    uint8_t byte1 = this->i2c_Buffer[4];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}

uint16_t BMS_IC::getPFStatusAB()
{
    this->_SMBus.read_Word(CMD_PFStatus, 4);
    uint8_t byte0 = this->i2c_Buffer[1];
    uint8_t byte1 = this->i2c_Buffer[2];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}
uint16_t BMS_IC::getPFStatusCD()
{
    this->_SMBus.read_Word(CMD_PFStatus, 5);
    uint8_t byte0 = this->i2c_Buffer[3];
    uint8_t byte1 = this->i2c_Buffer[4];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}

uint8_t BMS_IC::getcheckFETStatus()
{
    int FET_EN_Status = 0;
    FET_EN_Status = (getManufacturingStatus() >> 4) & 1;
    return FET_EN_Status;
}

uint16_t BMS_IC::getSerialNumber()
{
    this->_SMBus.read_Word(Register_SerialNumber, 2);
    uint8_t byte0 = this->i2c_Buffer[0];
    uint8_t byte1 = this->i2c_Buffer[1];
    uint16_t value = (byte1 << 8) | byte0;
    return value;
}

void BMS_IC::setBmsUndervoltageProtection()
{
    _SMBus.bmsUndervoltageProtection();
    delay(50);
}

void BMS_IC::setBmsShutdown()
{
    _SMBus.bmsShutdown();
    delay(50);
}

uint32_t BMS_IC::setRESET()
{
    _SMBus.write_dWord_BE(MBA_Command, 0x02410022); // 0x02410022 -> Big-Endian-Reihenfolge -> 0xE8002202
    delay(10000);
    return 0;
}

uint32_t BMS_IC::setFET_EN()
{
    _SMBus.write_dWord_BE(MBA_Command, 0x022200E8); // 0x022200E8 -> Big-Endian-Reihenfolge -> 0xE8002202
    delay(50);
    return 0;
}

uint16_t BMS_IC::setDesignVoltage()
{ // nur zum Testen
    _SMBus.write_Word(DesignVoltage, 14800);
    delay(50);
    return 0;
}

/*
int16_t BMS_IC::getAvgCurrent(){
    this->_SMBus.read_Word(Register_AvgCurrent, 2);
    this->AvgCurrent = this->i2c_Buffer[0];
    this->AvgCurrent |= (this->i2c_Buffer[1] << 8);
    return this->AvgCurrent;
}

uint8_t BMS_IC::getASOC(){
    this->_SMBus.read_Word(Register_ASOC, 1);
    this->ASOC = this->i2c_Buffer[0];
    return this->ASOC;
}

uint16_t BMS_IC::getBatteryStatus(){
    this->_SMBus.read_Word(Register_BatteryStatus, 2);
    this->BatteryStatus = this->i2c_Buffer[0];
    this->BatteryStatus |= (this->i2c_Buffer[1] << 8);
    return this->BatteryStatus;
}


uint16_t BMS_IC::getFirmwareVersion(){
    return this->FirmwareVersion;
}

uint16_t BMS_IC::getHardwareVersion(){
    return this->HarwareVersion;
}

uint16_t BMS_IC::getInstr_Flash_Signature(){
    return this->Instr_Flash_Signature;
}

uint16_t BMS_IC::getStatic_DF_Signature(){
    return this->Static_DF_Signature;
}

uint16_t BMS_IC::getChemicalID(){
    this->_SMBus.write_Block(CMD_Chemical_ID);
    delay(50);
    this->_SMBus.read_Block(2);
    this->Chemical_ID  = this->i2c_Buffer[3];
    this->Chemical_ID |= (this->i2c_Buffer[4] << 8);
    return this->Chemical_ID;
}

uint32_t BMS_IC::getOperationStatus(){
    this->_SMBus.write_Block(CMD_OperationStatus);
    delay(50);
    this->_SMBus.read_Block(4);
    this->OperationStatus  = this->i2c_Buffer[3];
    this->OperationStatus |= (this->i2c_Buffer[4] << 8);
    this->OperationStatus |= (this->i2c_Buffer[5] << 16);
    this->OperationStatus |= (this->i2c_Buffer[6] << 24);
    return this->OperationStatus;
}

// Charging status wegen 23 Bit etwas unklar
uint32_t BMS_IC::getChargingStatus(){
    this->_SMBus.write_Block(CMD_ChargingStatus);
    delay(50);
    this->_SMBus.read_Block(3);
    this->ChargingStatus  = this->i2c_Buffer[3];
    this->ChargingStatus |= (this->i2c_Buffer[4] << 8);
    this->ChargingStatus |= (this->i2c_Buffer[5] << 16);
    this->ChargingStatus |= (this->i2c_Buffer[6] << 24);
    return this->ChargingStatus;
}

// Unklar wegen GaugingStatus (0x56) <--> GaugingStatus 1 bis 3 (0x73 - 0x75)
uint32_t BMS_IC::getGaugingStatus(){
    this->_SMBus.write_Block(CMD_GaugingStatus);
    delay(50);
    this->_SMBus.read_Block(4);
    this->GaugingStatus  = this->i2c_Buffer[3];
    this->GaugingStatus |= (this->i2c_Buffer[4] << 8);
    this->GaugingStatus |= (this->i2c_Buffer[5] << 8);
    this->GaugingStatus |= (this->i2c_Buffer[6] << 8);
    return this->GaugingStatus;
}

uint16_t BMS_IC::getManufacturingStatus(){
    this->_SMBus.write_Block(CMD_ManufacturingStatus);
    delay(50);
    this->_SMBus.read_Block(2);
    this->ManufacturingStatus  = this->i2c_Buffer[3];
    this->ManufacturingStatus |= (this->i2c_Buffer[4] << 8);
    return this->ManufacturingStatus;
}

uint16_t BMS_IC::getStateOfHealth_FCC(){
    this->_SMBus.write_Block(CMD_StateofHealth);
    delay(50);
    this->_SMBus.read_Block(4);
    this->StateofHealth_FCC  = this->i2c_Buffer[3];
    this->StateofHealth_FCC |= (this->i2c_Buffer[4] << 8);
    return this->StateofHealth_FCC;
}

uint16_t BMS_IC::getStateOfHealth_Energy(){
    this->_SMBus.write_Block(CMD_StateofHealth);
    delay(50);
    this->_SMBus.read_Block(4);
    this->StateofHealth_Energy  = this->i2c_Buffer[5];
    this->StateofHealth_Energy |= (this->i2c_Buffer[7] << 8);
    return this->StateofHealth_Energy;
}
*/