/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Driver to read from Bluerobotics (TSYS01 based) temperature sensor via I2C
 */

#include <driver/bluerobo/CTSYS01.h>
#include <math.h>
#include "sensor_config.h"
#include <msp430.h>

CTSYS01::~CTSYS01()
{
    // TODO Auto-generated destructor stub
}

uint8_t CTSYS01::getParameter(){
    return 0x01;
}

uint8_t CTSYS01::getExternParameter(){
    return 0xFF;
}

uint32_t CTSYS01::getVersion(){
    return blue_robotics_temperature_celsius_fast_response;
}

bool CTSYS01::setCalib(float cal, uint8_t coeffToSet)
{
    //no calibration possible until now
    switch (coeffToSet){
    case 1:
        Co1 = cal;
        break;
    case 2:
        Co2 = cal;
        break;
    case 3:
        Co3 = cal;
        break;
    case 4:
        Co4 = cal;
        break;
    case 5:
        Co5 = cal;
        break;
    }
    calibrated = true;
    return true;
}

bool CTSYS01::getCalibrated()
{
    return calibrated;
}

bool CTSYS01::hibernate(){
    //this sensor can not hibernate, it just consumed low power, if not ad-converting
    return true;
}

bool CTSYS01::wakeup(){
    //this sensor can not wake up, it is ready after init
    return true;
}

bool CTSYS01::init() {
    // Reset the TSYS01, per datasheet
    uint8_t data[3];
    data[0] = TSYS01_RESET;
    if (!sendBytes_i2c(1, data))
        return false;
    // Wait for reset to complete
    __delay_cycles(10000);

//    // Read calibration values
//    for ( uint8_t i = 0 ; i < 8 ; i++ ) {
//        _i2cPort->beginTransmission(TSYS01_ADDR);
//        _i2cPort->write(TSYS01_PROM_READ+i*2);
//        _i2cPort->endTransmission();
//        _i2cPort->requestFrom(TSYS01_ADDR,2);
//        C[i] = (_i2cPort->read() << 8) | _i2cPort->read();
//    }

//    const uint16_t Co[8]; //C[7] is just used for CRC check
//        Co[1] = 28446;  //0xA2 K4
//        Co[2] = 24926;  //0XA4 k3
//        Co[3] = 36016;  //0XA6 K2
//        Co[4] = 32791;  //0XA8 K1
//        Co[5] = 40781;  //0XAA K0

    return true;
}
bool CTSYS01::startConversion(){
    uint8_t data[3];
    D1_temp = 0;
    data[0] = TSYS01_ADC_TEMP_CONV;
    if(!sendBytes_i2c(1, data)){
        return false;}

    __delay_cycles(180000);

    data[0] = TSYS01_ADC_READ;
    if(!sendBytes_i2c(1, data))
        return false;

    if(!readBytes_i2c(3, data))
         return false;
    D1_temp = data[0];
    D1_temp = (D1_temp<<8) | data[1];
    //ignore byte 0, because we divide by 256

    return true;
}

bool CTSYS01::getCalculatedValue(int64_t *aval)
{
    uint64_t adc = D1_temp;

//    float TEMP = (-2) * float(Co1) / 1000000000000000000000.0f * pow(adc,4) +
//        4 * float(Co2) / 10000000000000000.0f * pow(adc,3) +
//      (-2) * float(Co3) / 100000000000.0f * pow(adc,2) +
//        1 * float(Co4) / 1000000.0f * adc +
//      (-1.5) * float(Co5) / 100 ;

    uint64_t adc2 = adc * adc;
    uint64_t adc3 = adc2 * adc;
    uint64_t adc4 = adc3 *adc;
    float TEMP =   (-2E-21) * float(Co5) *adc4 +
                  (4E-16) * float(Co4) *adc3+
             (-2E-11) * float(Co3) *adc2+
            (0.000001) * float(Co2) *adc+
          (-0.015) * float(Co1);


    //this needs to be calculated with Co from the init function, but code is to big without optimization! TODO
//    float TEMP = -0.000000000000000056892 * adc*adc*adc*adc +
//            0.0000000000099704 * adc*adc*adc +
//          -0.00000072032 * adc*adc +
//          0.032791 * adc +
//          -611.715 ;


    union {
        float float_variable;
        unsigned char temp_array[4];
      } u;
    u.float_variable = (float)TEMP;
    memcpy(&aval[0], u.temp_array, 4);

    return true;

}

bool CTSYS01::getRAWValue(int64_t *aval){
    aval[0]=D1_temp;
    return true;
}

//uint16_t TSYS01::getCal(short i){
//    return this->C[i];
//}

