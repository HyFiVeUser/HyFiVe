/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Driver to read from Bluerobotics (MS5837 based) pressure sensor via I2C
 */

#include <driver/bluerobo/CMS5837.h>
#include "sensor_config.h"

//CMS5837::CMS5837()
//{
//    // TODO Auto-generated constructor stub
//
//}

CMS5837::~CMS5837()
{
    // TODO Auto-generated destructor stub
}

uint8_t CMS5837::getParameter(){
    return 0x02;
}

uint8_t CMS5837::getExternParameter(){
    return 0xFF;
}

uint32_t CMS5837::getVersion(){
    return blue_robotics_pressure_bar30;
}

bool CMS5837::setCalib(float cal, uint8_t coeffToSet)
{
    //no calibration possible until now
    switch (coeffToSet){
    case 1:
        Co[0] = cal;
        break;
    case 2:
        Co[1] = cal;
        break;
    case 3:
        Co[2] = cal;
        break;
    case 4:
        Co[3] = cal;
        break;
    case 5:
        Co[4] = cal;
        break;
    case 6:
        Co[5] = cal;
        break;
    }
    return true;
}

bool CMS5837::getCalibrated()
{
    //sensor does not need to get calibrated
    return true;
}

bool CMS5837::init()
{
    __delay_cycles(1000000);
    uint8_t data[3];
    data[0] = MS5837_RESET;
    if (!sendBytes_i2c(1, data))
        return false;
    // Wait for reset to complete
    __delay_cycles(100000);

    //read the constants from sensor and cache them
    //read all 7 values (2 bytes / 16 bits each)
    //first byte is                 'CRC  ' Factory defined version'
    //then constants are following  'C1                            '
    //                              'C2                            '
    //                              '..                            '
    //                              'C6                            '
    for ( uint8_t i = 0 ; i < 7 ; i++ )
    {
        data[0] = MS5837_PROM_READ+i*2;
        if (!sendBytes_i2c(1, data))
            return false;
        if (!readBytes_i2c(3, data))
            return false;
        Co[i] = data[0] << 8 | data[1];
    }

    // Verify that data is correct with CRC
    uint8_t crcRead = Co[0] >> 12;
    uint8_t crcCalculated = crc4(Co);

    if ( crcCalculated != crcRead ) {
        return false; // CRC fail
    }

    uint8_t version = (Co[0] >> 5) & 0x7F; // Extract the sensor version from PROM Word 0

    // Set _model according to the sensor version
    if (version == MS5837_02BA01)
    {
        _model = MS5837_02BA01;
    }
    else if (version == MS5837_02BA21)
    {
        _model = MS5837_02BA21;
    }
    else if (version == MS5837_30BA26)
    {
        _model = MS5837_30BA26;
    }
    else
    {
        _model = MS5837_UNRECOGNISED;
    }
    // The sensor has passed the CRC check, so we should return true even if
    // the sensor version is unrecognised.
    // (The MS5637 has the same address as the MS5837 and will also pass the CRC check)
    // (but will hopefully be unrecognised.)

    //setModel(0); //WHY???


    return true;
}

bool CMS5837::hibernate(){
    //this sensor can not hibernate, it just consumed low power, if not ad-converting
    return true;
}

bool CMS5837::wakeup(){
    //this sensor can not wake up, it is ready after init
    return true;
}

bool CMS5837::startConversion(){
    uint8_t data[3];

    //convertPressure
    //Request D1 conversion

    data[0] = MS5837_CONVERT_D1_4096;
    if(!sendBytes_i2c(1, data))
        return false;

    //how long between these two i2c interactions? delay(20); // Max conversion time per datasheet
    __delay_cycles(20000);

    data[0] = MS5837_ADC_READ;
    if(!sendBytes_i2c(1, data))
        return false;

    if(!readBytes_i2c(3, data))
         return false;
    D1_pres = data[0];
    D1_pres = (D1_pres<<8) | data[1];
    D1_pres = (D1_pres<<8) | data[2];


    //convertTemperature
    data[0] = MS5837_CONVERT_D2_4096;
    if(!sendBytes_i2c(1, data))
        return false;

    //how long between these two i2c interactions? delay(20); // Max conversion time per datasheet
    __delay_cycles(20000);

    data[0] = MS5837_ADC_READ;
    if(!sendBytes_i2c(1, data))
        return false;

    if(!readBytes_i2c(3, data))
        return false;
    D2_temp = data[0];
    D2_temp = (D2_temp<<8) | data[1];
    D2_temp = (D2_temp<<8) | data[2];

    return true;
}

bool CMS5837::getRAWValue(int64_t *aval){
    aval[0] = D1_pres;
    aval[1] = D2_temp;
    return true;
}

bool CMS5837::getCalculatedValue(int64_t *aval){
    // Given C1-C6 and D1, D2, calculated TEMP and P
    // Do conversion first and then second order temp compensation

    int32_t dT    = 0;
    int64_t SENS  = 0;
    int64_t OFF   = 0;
    int32_t SENSi = 0;
    int32_t OFFi  = 0;
    int32_t Ti    = 0;
    int64_t OFF2  = 0;
    int64_t SENS2 = 0;

    int32_t TEMP;
    int32_t P;

    // Terms called
    dT = D2_temp-uint32_t(Co[5])*256l;
    if ( _model == MS5837_02BA01 || _model == MS5837_02BA21 ) {
        SENS = int64_t(Co[1])*65536l+(int64_t(Co[3])*dT)/128l;
        OFF  = int64_t(Co[2])*131072l+(int64_t(Co[4])*dT)/64l;
        P    = (D1_pres*SENS/(2097152l)-OFF)/(32768l);
    } else {
        SENS = int64_t(Co[1])*32768l+(int64_t(Co[3])*dT)/256l;
        OFF  = int64_t(Co[2])*65536l+(int64_t(Co[4])*dT)/128l;
        P    = (D1_pres*SENS/(2097152l)-OFF)/(8192l);
    }

    // Temp conversion
    TEMP = 2000l+int64_t(dT)*Co[6]/8388608LL;

    //Second order compensation
    if ( _model == MS5837_02BA01 || _model == MS5837_02BA21 ) {
        if((TEMP/100)<20){         //Low temp
            Ti      = (11*int64_t(dT)*int64_t(dT))/(34359738368LL);
            OFFi    = (31*(TEMP-2000)*(TEMP-2000))/8;
            SENSi   = (63*(TEMP-2000)*(TEMP-2000))/32;
        }
    } else {
        if((TEMP/100)<20){         //Low temp
            Ti    = (3*int64_t(dT)*int64_t(dT))/(8589934592LL);
            OFFi  = (3*(TEMP-2000)*(TEMP-2000))/2;
            SENSi = (5*(TEMP-2000)*(TEMP-2000))/8;

            if((TEMP/100)<-15){    //Very low temp
                OFFi  = OFFi +7*(TEMP+1500l)*(TEMP+1500l);
                SENSi = SENSi+4*(TEMP+1500l)*(TEMP+1500l);
            }
        }
        else if((TEMP/100)>=20){    //High temp
            Ti    =  2*(dT*dT)/(137438953472LL);
            OFFi  = (1*(TEMP-2000)*(TEMP-2000))/16;
            SENSi = 0;
        }
    }

    OFF2  = OFF-OFFi;           //Calculate pressure and temp second order
    SENS2 = SENS-SENSi;

    TEMP = (TEMP-Ti);

    if ( _model == MS5837_02BA01 || _model == MS5837_02BA21 ) {
        P = (((D1_pres*SENS2)/2097152l-OFF2)/32768l);
    } else {
        P = (((D1_pres*SENS2)/2097152l-OFF2)/8192l);
    }

    union {
        float float_variable;
        unsigned char temp_array[4];
      } u;

    u.float_variable = (float)P;
    memcpy(&aval[0], u.temp_array, 4);


    u.float_variable = (float)TEMP;
    //aval[1] = temp_array;
    memcpy(&aval[1], u.temp_array, 4);



    return true;
}


uint8_t CMS5837::crc4(uint16_t n_prom[]) {
    uint16_t n_rem = 0;

    n_prom[0] = ((n_prom[0]) & 0x0FFF);
    n_prom[7] = 0;

    for ( uint8_t i = 0 ; i < 16; i++ ) {
        if ( i%2 == 1 ) {
            n_rem ^= (uint16_t)((n_prom[i>>1]) & 0x00FF);
        } else {
            n_rem ^= (uint16_t)(n_prom[i>>1] >> 8);
        }
        for ( uint8_t n_bit = 8 ; n_bit > 0 ; n_bit-- ) {
            if ( n_rem & 0x8000 ) {
                n_rem = (n_rem << 1) ^ 0x3000;
            } else {
                n_rem = (n_rem << 1);
            }
        }
    }

    n_rem = ((n_rem >> 12) & 0x000F);

    return n_rem ^ 0x00;
}
