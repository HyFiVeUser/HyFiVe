/*
 * PresensOXY.h
 *
 *  Created on: 07.09.2023
 *      Author: nhansen
 */

#ifndef DRIVER_PRESENSOXY_PRESENSOXY_H_
#define DRIVER_PRESENSOXY_PRESENSOXY_H_

#include "driver/Csensori2c.h"

class PresensOXY : public Csensor_i2c
{
public:
    PresensOXY(uint8_t address) : Csensor_i2c(address) {}; //use constructor of base class;
    virtual ~PresensOXY();
    virtual bool init();
    virtual bool hibernate();
    virtual bool wakeup();
    virtual bool startConversion();
    virtual bool getRAWValue(int64_t *aval);
    virtual bool getCalculatedValue(int64_t *aval);
    virtual uint8_t getParameter();
    virtual uint8_t getExternParameter();
    virtual uint8_t getVersion();
};

#endif /* DRIVER_PRESENSOXY_PRESENSOXY_H_ */
