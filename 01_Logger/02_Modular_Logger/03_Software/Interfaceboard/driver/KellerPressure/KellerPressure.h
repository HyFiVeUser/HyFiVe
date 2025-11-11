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

#ifndef DRIVER_KELLERPRESSURE_KELLERPRESSURE_H_
#define DRIVER_KELLERPRESSURE_KELLERPRESSURE_H_

#include "driver/Csensori2c.h"

class KellerPressure : public Csensor_i2c
{
public:
    static constexpr float Pa = 100.0f;
    static constexpr float bar = 0.001f;
    static constexpr float mbar = 1.0f;

    KellerPressure(uint8_t address) : Csensor_i2c(address) {}; //use constructor of base class
    virtual ~KellerPressure();
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

    /** Provide the density of the working fluid in kg/m^3. Default is for
     * seawater. Should be 997 for freshwater.
     */
    void setFluidDensity(float density);

    /** The read from I2C takes up for 40 ms, so use sparingly is possible.
     */
    bool read();

    /** Checks if the attached sensor is connectored or not. */
    bool status();

    /** Returns current range of the attached sensor. */
    float range();

    /** Pressure returned in mbar or mbar*conversion rate.
     */
    float pressure(float conversion = 1.0f);

    /** Temperature returned in deg C.
     */
    float temperature();

    /** Depth returned in meters (valid for operation in incompressible
     *  liquids only. Uses density that is set for fresh or seawater.
     */
    float depth();

    /** Altitude returned in meters (valid for operation in air only).
     */
    float altitude();

    /** Return true if the sensor has been initialized and detected. */
    bool isInitialized();

    uint16_t equipment;
    uint16_t place;
    uint16_t file;

    uint8_t mode;
    uint16_t year;
    uint8_t month;
    uint8_t day;

    uint32_t code;

    uint16_t P;
    uint16_t T;
    float P_bar;
    float P_mode;
    float P_min;
    float P_max;

private:
    //TwoWire     m_i2c;
    //KellerLD    m_keller;
    float fluidDensity;
    float T_degc;

    uint16_t cust_id0;
    uint16_t cust_id1;

    uint16_t readMemoryMap(uint8_t mtp_address);
};

#endif /* DRIVER_KELLERPRESSURE_KELLERPRESSURE_H_ */
