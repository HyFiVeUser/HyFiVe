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

#include <string>
#include "driver/pyro/pyroPicoO2.h"
#include "driverlib.h"
#include "sensor_config.h"

pyroPicoO2::~pyroPicoO2()
{
    // TODO Auto-generated destructor stub
}

uint8_t pyroPicoO2::getParameter()
{
    return 0x03;
}

uint8_t pyroPicoO2::getExternParameter()
{
    return 0x01;
}

uint32_t pyroPicoO2::getVersion()
{
    return (pyroscience_oxygen_oxycap_sub) | (pyroscience_oxygen_oxycap_hs_sub << 8);
}

bool pyroPicoO2::init()
{
    bool bResult = false;

    // initialize UART
    // Configure UART
    // SMCLK = 8MHz, Baudrate = 19200
    // UCBRx = 26, UCBRFx = 1, UCBRSx = 0, UCOS16 = 1
    EUSCI_A_UART_initParam param = {0};
    param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    param.clockPrescalar = 26;
    param.firstModReg = 1;
    param.secondModReg = 0;
    param.parity = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode = EUSCI_A_UART_MODE;
    param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION;
    bResult = uart.initUART(param);

    __delay_cycles(100000); // start up time

    //    std::string  c = "L,1\r";
    //    strcpy(commandToSend, c.c_str());
    //    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
    //    __delay_cycles(10000);
    //    while (!(uart.readBytes_uart(sizeof(answer), (uint8_t *)answer)) );
    //
    //    c = "K,1.0\r";
    //    strcpy(commandToSend, c.c_str());
    //    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
    //    __delay_cycles(10000);
    //    while (!(uart.readBytes_uart(sizeof(answer), (uint8_t *)answer)) );
    //
    //    c = "K,?\r";
    //    strcpy(commandToSend, c.c_str());
    //    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
    //    __delay_cycles(1000);
    //    while (!(uart.readBytes_uart(sizeof(version), (uint8_t *)version)) );
    //
    //
    //    c = "C,0\r"; //disable continuous mode
    //    strcpy(commandToSend, c.c_str());
    //    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
    //    __delay_cycles(1000);
    //    while (!(uart.readBytes_uart(sizeof(version), (uint8_t *)version)) );

    return bResult;
}

bool pyroPicoO2::setCalib(float cal, uint8_t coeffToSet)
{
    // no calibration possible until now
    return true;
}

bool pyroPicoO2::getCalibrated()
{
    // sensor does not need to get calibrated
    return true;
}

bool pyroPicoO2::hibernate()
{
    std::string c = "#STOP\r";
    strcpy(commandToSend, c.c_str());
    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
    uart.readBytes_uart(sizeof(answer), (uint8_t *)answer, 500);
    return true;
}

bool pyroPicoO2::wakeup()
{
    std::string c = "\r";
    strcpy(commandToSend, c.c_str());
    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);
    uart.readBytes_uart(sizeof(answer), (uint8_t *)answer, 500);
    return true;
}

bool pyroPicoO2::startConversion()
{
    return true;
}

bool pyroPicoO2::getRAWValue(int64_t *aval)
{

    // std::string c = "RT," + String(t);
    std::string c = "MEA 1 45\r"; // 1 (optical channel) + 4 (ambient air pressure) + 8 (relative humidity inside case) + 32 (case temp)
    strcpy(commandToSend, c.c_str());
    uart.resetReads_uart();

    uart.sendBytes_uart(c.length(), (uint8_t *)commandToSend);

    uint8_t data[150];
    for (uint8_t i = 0; i < sizeof(data); i++) // make sure, we will have a NULL terminated string afterwards
    {
        data[i] = 0;
    }
    __delay_cycles(1000);

    if (uart.readBytes_uart(sizeof(data), (uint8_t *)data, 450))
    {
        // searching for measurement command tag, receiving pointer to corresponding segment of c string
        char *pch = strstr((const char *)data, "MEA");
        // float val; //oxygenUMolar member!
        const char delim[2] = " ";
        if (pch != NULL)
        {
            // create array, to buffer single measurment values
            char measArray[22][20];
            // start to cut string into pieces
            char *pch2 = strtok(pch, delim); // is equal to pch
            uint8_t i = 0;
            while (pch2 != NULL)
            {
                strcpy(measArray[i], pch2);
                pch2 = strtok(NULL, delim); // getting new pointer to next segment
                i++;
            }
            // setting relevant measurement outputs
            int interpreteErrInt = (atoi(measArray[4 - 1]));
            dphi = (atoi(measArray[4 + 1 - 1]));
            val = strtof(measArray[4 + 2 - 1], NULL) * 0.001;
            float oxygenMBar = strtof(measArray[4 + 3 - 1], NULL) * 0.001;
            float humidityInCase = strtof(measArray[4 + 10 - 1], NULL) * 0.001;
            float tempSample = strtof(measArray[4 + 5 - 1], NULL) * 0.001;
            float tempCase = strtof(measArray[4 + 6 - 1], NULL) * 0.001;
        }

        // dphi: Phase shift of optical measurement (raw data)
        // for now we send -1, so valid raw value
        int64_t valInt = -1; // dphi;
        aval[0] = valInt;
        return true;
    }
    else
    {
        return false;
    }
}

bool pyroPicoO2::getCalculatedValue(int64_t *aval)
{
    // send mol/l
    union
    {
        float float_variable;
        unsigned char temp_array[4];
    } u;
    u.float_variable = val;
    memcpy(&aval[0], u.temp_array, 4);

    return true;
}

void pyroPicoO2::setTemperature(int32_t sampleTemp = -299999)
{

    // std::string c = "WTM 1 0 0 1 " + std::to_string((sampleTemp)) + "\r";
    snprintf(commandToSend, sizeof(commandToSend), "WTM 1 0 0 1 %d\r", sampleTemp * 100);
    uart.resetReads_uart();
    uart.sendBytes_uart(strlen(commandToSend), (uint8_t *)commandToSend);
    uint8_t data[150];
    __delay_cycles(1000);
    uart.readBytes_uart(sizeof(data), (uint8_t *)data, 500);

    // check if return message is equal to sent command TODO:
}

bool pyroPicoO2::writeSampleSal(int32_t sampleSal)
{
    // std::string c = "WTM 1 0 2 1 " + std::to_string((sampleSal)) + "\r";
    snprintf(commandToSend, sizeof(commandToSend), "WTM 1 0 2 1 %d\r", sampleSal);
    // strcpy(commandToSend, c.c_str());
    uart.sendBytes_uart(strlen(commandToSend), (uint8_t *)commandToSend);
    __delay_cycles(1000);
    uart.readBytes_uart(sizeof(answer), (uint8_t *)answer, 500);

    // check if return message is equal to sent command TODO:

    return true;
}
