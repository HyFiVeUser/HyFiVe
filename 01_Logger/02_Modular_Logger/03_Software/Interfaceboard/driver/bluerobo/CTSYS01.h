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
// Temperatursensor TSYS01

#ifndef DRIVER_BLUEROBO_CTSYS01_H_
#define DRIVER_BLUEROBO_CTSYS01_H_

#include "driver/Csensori2c.h"

class CTSYS01 : public Csensor_i2c
{
public:
    CTSYS01(uint8_t address) : Csensor_i2c(address) {}; //use constructor of base class
    virtual bool init();
    virtual bool hibernate();
    virtual bool wakeup();
    virtual bool startConversion();
    virtual bool getRAWValue(int64_t *aval);
    virtual bool getCalculatedValue(int64_t *aval);
    virtual uint8_t getParameter();
    virtual uint8_t getExternParameter();
    virtual uint32_t getVersion();
    virtual bool setCalib(float cal, uint8_t coeffToSet);
    virtual bool getCalibrated();
    virtual ~CTSYS01();

private:

    uint8_t slaveAddress = 0;
    uint64_t D1_temp;
    int32_t TEMP;
    bool calibrated = false;

    const uint8_t  TSYS01_RESET                       = 0x1E;
    const uint8_t  TSYS01_ADC_READ                    = 0x00;
    const uint8_t  TSYS01_PROM_READ                   = 0xA0;
    const uint8_t  TSYS01_ADC_TEMP_CONV               = 0x48;

    //float Co5 = 28446;  //0xA2 K4
    //float Co4 = 24926;  //0XA4 k3
    //float Co3 = 36016;  //0XA6 K2
    //float Co2 = 32791;  //0XA8 K1
    //float Co1 = 40781;  //0XAA K0

    float Co5 = 0;  //0xA2 K4
    float Co4 = 0;  //0XA4 k3
    float Co3 = 0;  //0XA6 K2
    float Co2 = 0;  //0XA8 K1
    float Co1 = 35000;  //0XAA K0
    //Ausgabe ohne Cal: ca -525
};

#endif /* DRIVER_BLUEROBO_CTSYS01_H_ */
