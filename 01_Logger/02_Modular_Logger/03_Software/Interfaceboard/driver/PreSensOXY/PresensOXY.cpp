/*
 * PresensOXY.cpp
 *
 *  Created on: 07.09.2023
 *      Author: nhansen
 */

#include <driver/PreSensOXY/PresensOXY.h>
#include <msp430fr2673.h>
#include "driverlib.h"

PresensOXY::~PresensOXY()
{
    // TODO Auto-generated destructor stub
}

uint8_t PresensOXY::getParameter(){
    return 0xFF;
}


uint8_t PresensOXY::getExternParameter(){
    return 0xFF;
}

uint8_t PresensOXY::getVersion(){
    return 0xFF;
}

bool PresensOXY::init()
{
    return true;
}

bool PresensOXY::hibernate()
{
    return true;
}

bool PresensOXY::wakeup()
{
    return true;
}

bool PresensOXY::startConversion()
{
    return true;
}

bool PresensOXY::getRAWValue(int64_t *aval)
{
    return true;
}

bool PresensOXY::getCalculatedValue(int64_t *aval)
{
    return true;
}
