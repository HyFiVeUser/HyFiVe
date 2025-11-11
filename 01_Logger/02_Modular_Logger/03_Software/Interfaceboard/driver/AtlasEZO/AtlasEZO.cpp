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

#include <string>
#include "driver/AtlasEZO/AtlasEZO.h"
#include "driverlib.h"
#include "sensor_config.h"

AtlasEZO::~AtlasEZO()
{
    // TODO Auto-generated destructor stub
}

uint8_t AtlasEZO::getParameter(){
    return 0x04;
}

uint8_t AtlasEZO::getExternParameter(){
    return 0xFF;
}
//uint32_t AtlasEZO::getVersion(){
//        return myID;
//}

uint32_t AtlasEZO::getVersion(){
    return (atlas_scientific_conductivity_k01 ) | (atlas_scientific_conductivity_k10 << 8);
}

bool AtlasEZO::setCalib(float cal, uint8_t coeffToSet)
{
    //no calibration possible until now
    return true;
}

bool AtlasEZO::getCalibrated()
{
    //sensor does not need to get calibrated
    return true;
}

bool AtlasEZO::init()
{
    bool bResult = false;

    // initialize UART
    //Configure UART
    //SMCLK = 8MHz, Baudrate = 9600
    //UCBRx = 52, UCBRFx = 1, UCBRSx = 0, UCOS16 = 1
    EUSCI_A_UART_initParam param = {0};
    param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    param.clockPrescalar = 52;
    param.firstModReg = 1;
    param.secondModReg = 0;
    param.parity = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode = EUSCI_A_UART_MODE;
    param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;

    bResult = uart.initUART(param);

    __delay_cycles(10000000);                    // start up time

    std::string  c = "L,1\r";
    strcpy(commandToSend, c.c_str());
    uart.resetReads_uart();
    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
    __delay_cycles(10000);
    uart.readBytes_uart(sizeof(answer), (uint8_t *)answer,500 );

    c = "K,?\r";
    //c = "K,0.1\r";
    strcpy(commandToSend, c.c_str());
    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
    __delay_cycles(10000);
    uart.readBytes_uart(sizeof(answer), (uint8_t *)answer,500 );

    if (strncmp(answer, "?K,1.0", 6) == 0)
        myID = atlas_scientific_conductivity_k10;
    else if (strncmp(answer, "?K,0.1", 6) == 0)
        myID = atlas_scientific_conductivity_k01;

    __delay_cycles(10000);

    c = "O,SG,0\r";
    strcpy(commandToSend, c.c_str());
    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
    __delay_cycles(1000);
    uart.readBytes_uart(sizeof(version), (uint8_t *)version,500 );

    __delay_cycles(10000);

    c = "O,S,0\r";
    strcpy(commandToSend, c.c_str());
    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
    __delay_cycles(1000);
    uart.readBytes_uart(sizeof(version), (uint8_t *)version,500 );

    __delay_cycles(10000);

    c = "O,TDS,0\r";
    strcpy(commandToSend, c.c_str());
    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
    __delay_cycles(1000);
    uart.readBytes_uart(sizeof(version), (uint8_t *)version,500 );

    __delay_cycles(10000);

    c = "C,0\r"; //disable continuous mode
    strcpy(commandToSend, c.c_str());
    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
    __delay_cycles(1000);
    uart.readBytes_uart(sizeof(version), (uint8_t *)version,500 );

    return bResult;
}

bool AtlasEZO::hibernate()
{
    std::string c = "Sleep\r";
    strcpy(commandToSend, c.c_str());
    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
    uart.readBytes_uart(sizeof(answer), (uint8_t *)answer,500 );
    return true;
}

bool AtlasEZO::wakeup()
{
    std::string c = "WA\r"; //any command wakes device up
    strcpy(commandToSend, c.c_str());
    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
    uart.readBytes_uart(sizeof(answer), (uint8_t *)answer,500 );
    return true;
}

bool AtlasEZO::startConversion()
{

    return true;
}

bool AtlasEZO::getRAWValue(int64_t *aval)
{
    bool bResult = false;
    // std::string c = "RT," + String(t);
    std::string c = "R\r";
    strcpy(commandToSend, c.c_str());
    uart.resetReads_uart();
    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);

    for (uint8_t i=0; i<sizeof(answer); i++) //make sure, we will have a NULL terminated string afterwards
    {
        answer[i] = 0;
    }
    if (!uart.readBytes_uart(sizeof(answer), (uint8_t *)answer, 700) )
    {
        return false;
    }
    val = 0;
    int potential = 0;
    int buffer;


        for (int i = 0; i <5 ; i++)
        {
            buffer = answer[i] - 48;
            val = val * 10;
            if (buffer == -2)
            {                       // decimal point
                potential = 1;
                val = val / 10;     // reverse last multiplication
            }
            else if (buffer <0)
            {                       // end of data
                val = val / 10;     // reverse last multiplication
                break;
            }
            else
            {
                val += buffer;
                if (potential >= 1)
                {
                    potential++;
                }
            }
        }
        for (int i = 1; i < potential; i++)
        {
            val /= 10;
        }
        //val is in uS/cm

        //convert to nS/cm
        //uint64_t valInt = val*1000;
        //no, we don't have any raw value, so we return -1
        int64_t valInt = -1;
        aval[0] = valInt;
        return true;

}

bool AtlasEZO::getCalculatedValue(int64_t *aval)
{
    //send mS/cm ( /1000)
    union {
        float float_variable;
        unsigned char temp_array[4];
    } u;
    u.float_variable = (float)val / 1000.0;
    memcpy(&aval[0], u.temp_array, 4);
    return true;
}

void AtlasEZO::k01sleep()
{
    std::string c = "Sleep\r";
    strcpy(commandToSend, c.c_str());
    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
}

