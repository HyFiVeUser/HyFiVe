/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Base Class for sensor driver (incl. I2C-Master)
 */

#ifndef DRIVER_CSENSORI2C_H_
#define DRIVER_CSENSORI2C_H_

#include <stdint.h>

    enum I2C_SensorTypes
    {
        UnknownType        = 0x00,
        TypeCMS5837        = 0x01,
        TypeCTSYS01        = 0x02,
        TypeAnalog         = 0x03,
        TypeAtlasEZO       = 0x04,
        TypeKellerPressure = 0x05,
        TypePreSensOXY     = 0x06,
    };

class Csensor_i2c
{
public:
    Csensor_i2c(uint8_t address);
    virtual bool init() = 0;
    virtual bool hibernate() = 0;
    virtual bool wakeup() = 0;
    virtual bool startConversion() = 0;
    virtual bool getRAWValue(int64_t *aval) = 0;
    virtual bool getCalculatedValue(int64_t *aval) = 0;
    virtual uint8_t getParameter() = 0;
    virtual uint8_t getExternParameter() = 0;
    virtual uint32_t getVersion() = 0;
    virtual bool setCalib(float cal, uint8_t coeffToSet) = 0;
    virtual bool getCalibrated() = 0;
    virtual ~Csensor_i2c();
//    virtual bool writeSampleTemp(int32_t sampleTemp) = 0;
//    virtual bool writeSampleSal(int32_t sampleSal) = 0;

    static __interrupt void USCI_B1_ISR( void );

    int32_t getTemperature(){return temperature;}
    void setTemperature(int32_t temp){temperature = temp;}

protected:
    bool sendBytes_i2c(uint8_t numberOfBytes, uint8_t *data);
    bool readBytes_i2c(uint8_t numberOfBytes, uint8_t *data);

private:
    uint8_t slaveAddress = 0;
    static uint8_t *PTxData;
    static uint8_t *PRxData;
    static uint8_t RxCount;
    static uint8_t TxCount;
    int16_t temperature = -9999;


};

#endif /* DRIVER_CSENSORI2C_H_ */
