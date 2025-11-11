/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Driver to read from Pyrosience picoO2 via UART
 */

#ifndef DRIVER_PYRO_PYROPICOO2_H_
#define DRIVER_PYRO_PYROPICOO2_H_

#include "driver/Csensori2c.h"
#include "driver/UART_drv.h"

#define K01CALCOEFF 11 // number of calibration coefficients

class pyroPicoO2 : public Csensor_i2c
{
public:
    pyroPicoO2(uint8_t address) : Csensor_i2c(address) {}; // use constructor of base class
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
    virtual ~pyroPicoO2();

    void setTemperature(int32_t sampleTemp);
    virtual bool writeSampleSal(int32_t sampleSal);

private:
    // void     k01sleep();
    UART_drv uart;
    char commandToSend[40];
    char version[10];
    char answer[40];
    char cal[K01CALCOEFF][12];
    int32_t dphi;
    float val;
};

#endif /* DRIVER_PYRO_PYROPICOO2_H_ */
