/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Driver to read from Keller Sensor via I2C
 */

#include <math.h>
#include <driver/KellerPressure/KellerPressure.h>
#include "sensor_config.h"

#define LD_REQUEST                  0xAC
#define LD_CUST_ID0                 0x00
#define LD_CUST_ID1                 0x01
#define LD_SCALING0                 0x12
#define LD_SCALING1                 0x13
#define LD_SCALING2                 0x14
#define LD_SCALING3                 0x15
#define LD_SCALING4                 0x16

//KellerPressure::KellerPressure()
//{
//    // TODO Auto-generated constructor stub
//}

KellerPressure::~KellerPressure()
{
//    delete m_keller;
//    delete m_i2c;
}

uint8_t KellerPressure::getParameter(){
    return 0x02;
}


uint8_t KellerPressure::getExternParameter(){
    return 0xFF;
}

uint32_t KellerPressure::getVersion(){
    return keller_pressure_series_20;
}

bool KellerPressure::setCalib(float cal, uint8_t coeffToSet)
{
    //no calibration possible until now
    return true;
}

bool KellerPressure::getCalibrated()
{
    //sensor does not need to get calibrated
    return true;
}

bool KellerPressure::init()
{
//    m_i2c = new TwoWire();
//    if (m_i2c == nullptr) {
//        return false;
//    }
//    m_keller = new KellerLD();
//    if (m_keller == nullptr) {
//        delete m_i2c;
//        return false;
//    }
//    m_keller.init(this);
//    return m_keller.isInitialized();

    fluidDensity = 1029;

    cust_id0 = readMemoryMap(LD_CUST_ID0);
    cust_id1 = readMemoryMap(LD_CUST_ID1);

    code = (uint32_t(cust_id1) << 16) | cust_id0;
    equipment = cust_id0 >> 10;
    place = cust_id0 & 0b000000111111111;
    file = cust_id1;

    uint16_t scaling0;
    scaling0 = readMemoryMap(LD_SCALING0);

    mode = scaling0 & 0b00000011;
    year = scaling0 >> 11;
    month = (scaling0 & 0b0000011110000000) >> 7;
    day = (scaling0 & 0b0000000001111100) >> 2;

    // handle P-mode pressure offset (to vacuum pressure)

    if (mode == 0) {
        // PA mode, Vented Gauge. Zero at atmospheric pressure
        P_mode = 1.01325;
    } else if (mode == 1) {
        // PR mode, Sealed Gauge. Zero at 1.0 bar
        P_mode = 1.0;
    } else {
        // PAA mode, Absolute. Zero at vacuum
        // (or undefined mode)
        P_mode = 0;
    }

    uint32_t scaling12 = (uint32_t(readMemoryMap(LD_SCALING1)) << 16) | readMemoryMap(LD_SCALING2);

    P_min = *reinterpret_cast<float*>(&scaling12);

    uint32_t scaling34 = (uint32_t(readMemoryMap(LD_SCALING3)) << 16) | readMemoryMap(LD_SCALING4);

    P_max = *reinterpret_cast<float*>(&scaling34);

    return isInitialized();
}

bool KellerPressure::hibernate()
{
    return true;
}

bool KellerPressure::wakeup()
{
    return true;
}

bool KellerPressure::startConversion()
{

    return read();
}

bool KellerPressure::getRAWValue(int64_t *aval)
{
    //for now we send -1, no valid raw value
    aval[0] = -1; //P;
    aval[1] = -1; //T;
    return true;
}

bool KellerPressure::getCalculatedValue(int64_t *aval)
{
    float fPressure = 0;

    fPressure = pressure(1);
    //*aval = static_cast<uint64_t>(fPressure);

    union {
        float float_variable;
        unsigned char temp_array[4];
    } u;
    u.float_variable = (float)fPressure;
    memcpy(&aval[0], u.temp_array, 4);

    u.float_variable = (float)temperature();
    memcpy(&aval[1], u.temp_array, 4);

    return true;
}

void KellerPressure::setFluidDensity(float density) {
    fluidDensity = density;
}

bool KellerPressure::read() {
    bool bResult;
    uint8_t answer[5];
    uint8_t status;
    uint8_t byteHi, byteLo = LD_REQUEST;

    bResult = sendBytes_i2c(1, &byteLo);

    __delay_cycles(200000); //25ms

    bResult = readBytes_i2c(sizeof(answer), answer);
    status = answer[0];
    byteHi = answer[1];
    byteLo = answer[2];
    P = (byteHi << 8) | byteLo;
    byteHi = answer[3];
    byteLo = answer[4];
    T = (byteHi << 8) | byteLo;

    P_bar = (float(P)-16384)*(P_max-P_min)/32768 + P_min + P_mode;
    T_degc = ((T>>4)-24)*0.05-50;

    return bResult;
}

uint16_t KellerPressure::readMemoryMap(uint8_t mtp_address) {
    bool bResult;
    uint8_t answer[3];
    uint8_t status;
    uint8_t byteHi, byteLo;

    bResult = sendBytes_i2c(1, &mtp_address);

    __delay_cycles(10000); // allow for response to come in

    bResult = readBytes_i2c(sizeof(answer), answer);
    status = answer[0];
    byteHi = answer[1];
    byteLo = answer[2];
    return ((byteHi << 8) | byteLo);
}

bool KellerPressure::status() {
    if (equipment <= 62 ) {
        return true;
    } else {
        return false;
    }
}

float KellerPressure::range() {
    return P_max-P_min;
}

float KellerPressure::pressure(float conversion) {
    return P_bar*1000.0f*conversion;
}

float KellerPressure::temperature() {
    return T_degc;
}

float KellerPressure::depth() {
    return (pressure(KellerPressure::Pa)-101325)/(fluidDensity*9.80665);
}

float KellerPressure::altitude() {
    return (1-pow((pressure()/1013.25),0.190284))*145366.45*.3048;
}

bool KellerPressure::isInitialized() {
    return (cust_id0 >> 10) != 63; // If not connected, equipment code == 63
}

