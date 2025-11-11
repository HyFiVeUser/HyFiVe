/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Driver to read from analog in. linear calibration function, see getCalculatedValue()
 */

#ifndef DRIVER_Analog_H_
#define DRIVER_Analog_H_

#include "driver/Csensori2c.h"
#include <msp430.h>
class Analog : public Csensor_i2c
{
public:
    Analog(uint8_t address) : Csensor_i2c(address) {}; //use constructor of base class
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
    virtual ~Analog();

private:
    uint8_t crc4(uint16_t n_prom[]);
    uint16_t ADC_Value;
    bool calibrated = false;
    //float Co1 = 0.0912;  //Absolut Offset
    //float Co2 = 44.6508;  //Calibration Factor
    //float Co3 = 36016;  //
    //float Co4 = 32791;  //
    //float Co5 = 40781;  //

    float Co1 = 20000;  //Absolut Offset
    float Co2 = 20000;  //Calibration Factor
    float Co3 = 0;  //
    float Co4 = 0;  //
    float Co5 = 0;  //
     // Augabe ca -399999904.00
};

#endif /* DRIVER_Analog_H_ */
