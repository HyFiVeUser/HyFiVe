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

#ifndef DRIVER_BLUEROBO_CMS5837_H_
#define DRIVER_BLUEROBO_CMS5837_H_

#include "driver/Csensori2c.h"

class CMS5837 : public Csensor_i2c
{
public:
    CMS5837(uint8_t address) : Csensor_i2c(address) {}; //use constructor of base class
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
    virtual ~CMS5837();

private:
    uint8_t crc4(uint16_t n_prom[]);

    uint16_t Co[8]; //C[7] is just used for CRC check
    uint64_t D1_pres, D2_temp;
    uint8_t _model;
    float fluidDensity;
    const uint8_t MS5837_UNRECOGNISED = 255;
    const uint8_t MS5837_02BA01 = 0x00; // Sensor version: From MS5837_02BA datasheet Version PROM Word 0
    const uint8_t MS5837_02BA21 = 0x15; // Sensor version: From MS5837_02BA datasheet Version PROM Word 0
    const uint8_t MS5837_30BA26 = 0x1A; // Sensor version: From MS5837_30BA datasheet Version PROM Word 0

    const uint8_t  MS5837_RESET                        = 0x1E;
    const uint8_t  MS5837_ADC_READ                     = 0x00;
    const uint8_t  MS5837_PROM_READ                    = 0xA0;
    const uint8_t  MS5837_CONVERT_D1_4096              = 0x48;
    const uint8_t  MS5837_CONVERT_D2_4096              = 0x58;

};

#endif /* DRIVER_BLUEROBO_CMS5837_H_ */
