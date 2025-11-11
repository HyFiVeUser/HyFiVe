/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Driver to read from AtlasEZO K0.1 / K1.0 Sensor via UART
 */

#ifndef DRIVER_ATLASEZO_ATLASEZO_H_
#define DRIVER_ATLASEZO_ATLASEZO_H_

#include "driver/Csensori2c.h"
#include "driver/UART_drv.h"

#define K01CALCOEFF 11         // number of calibration coefficients

class AtlasEZO : public Csensor_i2c
{
public:
    AtlasEZO(uint8_t address) : Csensor_i2c(address) {}; //use constructor of base class
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
    virtual ~AtlasEZO();

private:
    void     k01sleep();
    UART_drv uart;
    char     commandToSend[10];
    char     version[10];
    char     answer[40];
    char     cal[K01CALCOEFF][12];
    float    val;
    uint8_t  myID = 0;
};

#endif /* DRIVER_ATLASEZO_ATLASEZO_H_ */
