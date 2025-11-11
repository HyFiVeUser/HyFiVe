/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: main loop, callback functions for i2c slave interface
 */

#include <I2Cslave/i2c_slave.h>
#include <msp430.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "driver/Csensori2c.h"
#include "driver/bluerobo/CMS5837.h"
#include "driver/bluerobo/CTSYS01.h"
#include "driver/Analog/Analog.h"
#include "driver/AtlasEZO/AtlasEZO.h"
#include "driver/KellerPressure/KellerPressure.h"
#include "driver/pyro/pyroPicoO2.h"
#include "sensor_config.h"

#define MCLK_FREQ_MHZ 8 // MCLK = 8MHz
unsigned char getOwnI2CAddress(void);
void setPins(void);

/* callback for start condition */
void start_cb();

/* callback for bytes received */
void receive_cb(unsigned char receive);

/* callback to transmit bytes */
void transmit_cb(unsigned char volatile *byte);

/* last command */
volatile uint8_t cmd = CMD_UNKNOWN;

/* last parameter */
volatile uint8_t par[5] = {PAR_UNKNOWN, PAR_UNKNOWN, PAR_UNKNOWN, PAR_UNKNOWN, PAR_UNKNOWN};

/* response to send out on read req. */
volatile uint8_t res[8] = {RES_ERROR, RES_ERROR, RES_ERROR, RES_ERROR, RES_ERROR, RES_ERROR, RES_ERROR, RES_ERROR};

volatile uint8_t byteCount = 0;
uint8_t FW_VERSION = 0;
uint32_t version = 0x0;
uint8_t parameter = 0xFF;
uint8_t externparameter = 0xFF;
volatile int64_t values[2] = {0, 0};
volatile int64_t rawValues[2] = {0, 0};
volatile uint8_t startConversion = 1; // used as command flag and readyflag and errorflag
                                      // 0: conversion in progress
                                      // 1: nothing happening / ready
                                      // 2: fault in conversion
volatile int32_t lastTemperature = -2999; // last Temperature in centigrad
volatile bool setTemperature = false;
volatile bool setCalib = false;
volatile bool calibrated = false;
volatile bool goToSleep = false;
volatile bool wakeUp = false;
volatile bool sleepOrWarmup = false;
volatile uint8_t calibToSet = 0;
volatile float floatToSet = 0.0;

// Sensor WakeUp Time
uint16_t sensorWakeUpTime = 0;

void process_cmd(unsigned char cmd, unsigned char *par0)
{
    res[0] = RES_ERROR;

    union
    {
        float float_variable;
        unsigned char temp_array[4];
    } u;

    switch (cmd)
    {
    case CMD_PING:
        byteCount = 1;
        res[0] = RES_PONG;
        break;

    case CMD_GETVER:
        byteCount = 4;
        res[0] = version & 0xFF;
        res[1] = (version & 0xFF00) >> 8;
        ;
        res[2] = (version & 0xFF0000) >> 16;
        ;
        res[3] = (version & 0xFF000000) >> 24;
        ;
        break;

    case CMD_GET_PARAMETER:
        byteCount = 1;
        res[0] = parameter;
        break;

    case CMD_GET_EXTERNPARAMETER: // Neuer Case f�r zweiten Parameter
        byteCount = 1;
        res[0] = externparameter;
        break;

    case CMD_GET_RDY:
        byteCount = 1;
        // respond with 0 if something is in progress, 1 if ready, 2 is error
        if (setCalib || setTemperature || sleepOrWarmup) // if set calib or set Temperature is in progress, we are not ready
            res[0] = 0;
        else
            res[0] = startConversion;
        break;

    case CMD_GET_CALIBRATED:
        byteCount = 1;
        if (calibrated) // we are already calibrated
            res[0] = 1;
        else
            res[0] = 0;
        break;

    case CMD_GET_SENSOR_WAKEUP_TIME:
        byteCount = 2;
        res[0] = sensorWakeUpTime & 0xFF;
        res[1] = (sensorWakeUpTime >> 8) & 0xFF;
        break;

    case CMD_GET_FW_VERSION:
        byteCount = 1;
        res[0] = FW_VERSION;
        break;

    case CMD_GETVALUE1:
        byteCount = 8;
        res[0] = values[0] & 0xFF;
        res[1] = (values[0] & 0xFF00) >> 8;
        res[2] = (values[0] & 0xFF0000) >> 16;
        res[3] = (values[0] & 0xFF000000) >> 24;
        res[4] = (values[0] & 0xFF00000000) >> 32;
        res[5] = (values[0] & 0xFF0000000000) >> 40;
        res[6] = (values[0] & 0xFF000000000000) >> 48;
        res[7] = (values[0] & 0xFF00000000000000) >> 56;
        break;

    case CMD_GETVALUE2:
        byteCount = 8;
        res[0] = values[1] & 0xFF;
        res[1] = (values[1] & 0xFF00) >> 8;
        res[2] = (values[1] & 0xFF0000) >> 16;
        res[3] = (values[1] & 0xFF000000) >> 24;
        res[4] = (values[1] & 0xFF00000000) >> 32;
        res[5] = (values[1] & 0xFF0000000000) >> 40;
        res[6] = (values[1] & 0xFF000000000000) >> 48;
        res[7] = (values[1] & 0xFF00000000000000) >> 56;
        break;

    case CMD_GETRAWVALUE1:
        byteCount = 8;
        res[0] = rawValues[0] & 0xFF;
        res[1] = (rawValues[0] & 0xFF00) >> 8;
        res[2] = (rawValues[0] & 0xFF0000) >> 16;
        res[3] = (rawValues[0] & 0xFF000000) >> 24;
        res[4] = (rawValues[0] & 0xFF00000000) >> 32;
        res[5] = (rawValues[0] & 0xFF0000000000) >> 40;
        res[6] = (rawValues[0] & 0xFF000000000000) >> 48;
        res[7] = (rawValues[0] & 0xFF00000000000000) >> 56;
        break;

    case CMD_GETRAWVALUE2:
        byteCount = 8;
        res[0] = rawValues[1] & 0xFF;
        res[1] = (rawValues[1] & 0xFF00) >> 8;
        res[2] = (rawValues[1] & 0xFF0000) >> 16;
        res[3] = (rawValues[1] & 0xFF000000) >> 24;
        res[4] = (rawValues[1] & 0xFF00000000) >> 32;
        res[5] = (rawValues[1] & 0xFF0000000000) >> 40;
        res[6] = (rawValues[1] & 0xFF000000000000) >> 48;
        res[7] = (rawValues[1] & 0xFF00000000000000) >> 56;
        break;

    case CMD_CONVERT:
        startConversion = 0;
        break;

    case CMD_SET_TEMP:
        u.temp_array[0] = par[3];
        u.temp_array[1] = par[2];
        u.temp_array[2] = par[1];
        u.temp_array[3] = par[0];
        lastTemperature = u.float_variable * 10; // float to centi grade
        setTemperature = true;
        break;

    case CMD_SET_CALIB:
        u.temp_array[0] = par[4];
        u.temp_array[1] = par[3];
        u.temp_array[2] = par[2];
        u.temp_array[3] = par[1];
        calibToSet = par[0];
        floatToSet = u.float_variable;
        setCalib = true;
        break;

    case CMD_SENSOR_WAKEUP:
        wakeUp = true;
        res[0] = RES_PONG;
        break;

    case CMD_SENSOR_SLEEP:
        goToSleep = true;
        res[0] = RES_PONG;
        break;

    case CMD_SOFTWARE_RESET:
        WDTCTL = 0xDEAD;
        break;

    case CMD_GET_SENSORVOLTAGE:
        byteCount = 1;

#if SELECTED_SENSOR == 1
        res[0] = (1) | (0 << 1) | (0 << 2);

#elif SELECTED_SENSOR == 2
        res[0] = (1) | (0 << 1) | (0 << 2);

#elif SELECTED_SENSOR == 3
        res[0] = (1) | (0 << 1) | (0 << 2);

#elif SELECTED_SENSOR == 4
        res[0] = (1) | (0 << 1) | (0 << 2);

#elif SELECTED_SENSOR == 5
        res[0] = (1) | (0 << 1) | (0 << 2);

#elif SELECTED_SENSOR == 6
        res[0] = (1) | (1 << 1) | (0 << 2);

#endif

        break;

    default:
        res[0] = RES_ERROR;
    }
}

void start_cb()
{
    cmd = CMD_UNKNOWN;
    par[0] = PAR_UNKNOWN;
}

void receive_cb(unsigned char receive)
{
    // if command is still unknown, the byte should be a command
    if (cmd == CMD_UNKNOWN)
    {
        byteCount = 1;
        // save command in cmd
        cmd = receive;

        // process one byte commands:
        if (cmd == CMD_GETVER ||
            cmd == CMD_PING ||
            cmd == CMD_GETVALUE1 ||
            cmd == CMD_GETVALUE2 ||
            cmd == CMD_GETRAWVALUE1 ||
            cmd == CMD_GETRAWVALUE2 ||
            cmd == CMD_CONVERT ||
            cmd == CMD_GET_SENSORVOLTAGE ||
            cmd == CMD_SENSOR_WAKEUP ||
            cmd == CMD_SENSOR_SLEEP ||
            cmd == CMD_GET_PARAMETER ||
            cmd == CMD_GET_EXTERNPARAMETER ||
            cmd == CMD_GET_RDY ||
            cmd == CMD_GET_CALIBRATED ||
            cmd == CMD_GET_SENSOR_WAKEUP_TIME ||
            cmd == CMD_GET_FW_VERSION ||
            cmd == CMD_SOFTWARE_RESET)
        {
            process_cmd(cmd, (uint8_t *)par);
        }
        // else - cmd is known
    }
    else
    {
        byteCount++;
        // byte is a parameter of the command
        // process 2 byte commands (1 byte command, 1 byte parameter)
        if (cmd == CMD_1ByteDummyTest)
        {
            par[0] = receive;
            process_cmd(cmd, (uint8_t *)par);
        }
        else if (cmd == CMD_SET_TEMP)
        {
            // process 5 byte commands (1 byte command, 4 byte parameter)
            if (byteCount == 5)
            {
                par[3] = receive;
                process_cmd(cmd, (uint8_t *)par);
            }
            if (byteCount == 4)
            {
                par[2] = receive;
            }
            if (byteCount == 3)
            {
                par[1] = receive;
            }
            if (byteCount == 2)
                par[0] = receive;
        }
        else if (cmd == CMD_SET_CALIB)
        {
            // process 6 byte commands (1 byte command, 5 byte parameter)
            if (byteCount == 6)
            {
                par[4] = receive;
                process_cmd(cmd, (uint8_t *)par);
            }
            if (byteCount == 5)
            {
                par[3] = receive;
            }
            if (byteCount == 4)
            {
                par[2] = receive;
            }
            if (byteCount == 3)
            {
                par[1] = receive;
            }
            if (byteCount == 2)
                par[0] = receive;
        }
    }
}

void transmit_cb(unsigned char volatile *byte)
{
    if (byteCount > 0)
    {
        byteCount--;
        *byte = res[byteCount];
    }
    else
        *byte = RES_ERROR;
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD; // stop watchdog timer
    setPins();                // set input and output Pins
    // init slave interface with set i2c address (dip switches)
    I2C_slaveInit(start_cb, transmit_cb, receive_cb, getOwnI2CAddress());
    __bis_SR_register(GIE);

    // Select Sensor via SELECTED_SENSOR in sesnor_config.h

    // ID | Manufacturer        | Parameter                     | Model                          / sensor_type_ID     |
    //----:---------------------:-------------------------------:--------------------------------:--------------------:
    // 1  : blue_robotics       : pressure                      : bar30                          : 1                  :
    // 2  : blue_robotics       : temperature                   : celsius_fast_response          : 2                  :
    // 3  : keller              : pressure                      : series_20                      : 6                  :
    // 4  : atlas_scientific    : conductivity                  : k0.1 | k1.0                    : 3 | 10             :
    // 5  : pyroscience         : oxygen                        : oxycap_sub | oxycap_hs_sub     : 9 | 11             :
    // 6  : Turner              : turbidity | phycoerythrin     : C-Flour_TRB | C-Flour_PE       : 12 | 13            :

#if SELECTED_SENSOR == 1
    FW_VERSION = 1;
    sensorWakeUpTime = 1000;
    CMS5837::CMS5837 sensor(0x76);

#elif SELECTED_SENSOR == 2
    FW_VERSION = 1;
    sensorWakeUpTime = 1000;
    CTSYS01 sensor(0x77);

#elif SELECTED_SENSOR == 3
    FW_VERSION = 2;
    sensorWakeUpTime = 1000;
    KellerPressure sensor(0x40);

#elif SELECTED_SENSOR == 4
    FW_VERSION = 1;
    sensorWakeUpTime = 2000;
    AtlasEZO::AtlasEZO sensor(0);

#elif SELECTED_SENSOR == 5
    FW_VERSION = 1;
    sensorWakeUpTime = 1000;
    pyroPicoO2 sensor(0);

#elif SELECTED_SENSOR == 6
    FW_VERSION = 3;
    sensorWakeUpTime = 1000;
    Analog::Analog sensor(0);

#endif

    // initialize sensor driver, save version and parameter information to global variable
    sensor.init();
    version = sensor.getVersion();
    parameter = sensor.getParameter();
    externparameter = sensor.getExternParameter();
    calibrated = sensor.getCalibrated();

    while (1) // endless loop waiting for i2c command
    {
        LPM0; // Wait for stop bit in LPM0
        while (UCB0CTL1 & UCTXSTP)
            ; // Ensure stop condition exists
        // check I2C command

        if (goToSleep)
        {
            // mater wants us to put the sensor to low power mode
            // we switch the mosfet off:
            // signal that we are not ready anymore
            sleepOrWarmup = true;
            P3OUT &= ~BIT5;
            __delay_cycles(180000);
            P2OUT &= ~BIT7;
            __delay_cycles(180000);
            goToSleep = false;
        }
        if (wakeUp)
        {
            // we switch the mosfet on:
            P3OUT |= BIT5;
            __delay_cycles(180000);
            P2OUT |= BIT7;
            __delay_cycles(180000);
            // reinit sensor
            sensor.init();
            // wait for warm up time

            // signal that we are ready
            sleepOrWarmup = false;
            wakeUp = false;
        }
        if (setTemperature)
        {
            sensor.setTemperature(lastTemperature);
            setTemperature = false;
        }
        if (setCalib)
        {
            sensor.setCalib(floatToSet, calibToSet);
            calibrated = sensor.getCalibrated();
            setCalib = false;
        }
        if (startConversion == 0)
        {

            if (!sensor.startConversion())
            {
                startConversion = 2;
                // sensor.init();
                // WDTCTL = 0xDEAD;
            }

            if (!sensor.getRAWValue((int64_t *)rawValues)) // first get raw value, some drivers get value when this is called
            {
                startConversion = 2;
            }

            if (!sensor.getCalculatedValue((int64_t *)values)) // get calculated (calibrated) value
            {
                startConversion = 2;
            }

            if (startConversion == 0) // if all was good, we set to 1, -> values are ready
            {
                startConversion = 1;
            }
        }
    }
    // return 0;
}

void Software_Trim()
{
    unsigned int oldDcoTap = 0xffff;
    unsigned int newDcoTap = 0xffff;
    unsigned int newDcoDelta = 0xffff;
    unsigned int bestDcoDelta = 0xffff;
    unsigned int csCtl0Copy = 0;
    unsigned int csCtl1Copy = 0;
    unsigned int csCtl0Read = 0;
    unsigned int csCtl1Read = 0;
    unsigned int dcoFreqTrim = 3;
    unsigned char endLoop = 0;

    do
    {
        CSCTL0 = 0x100; // DCO Tap = 256
        do
        {
            CSCTL7 &= ~DCOFFG; // Clear DCO fault flag
        } while (CSCTL7 & DCOFFG); // Test DCO fault flag

        __delay_cycles((unsigned int)3000 * MCLK_FREQ_MHZ); // Wait FLL lock status (FLLUNLOCK) to be stable
                                                            // Suggest to wait 24 cycles of divided FLL reference clock
        while ((CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1)) && ((CSCTL7 & DCOFFG) == 0))
            ;

        csCtl0Read = CSCTL0; // Read CSCTL0
        csCtl1Read = CSCTL1; // Read CSCTL1

        oldDcoTap = newDcoTap;                    // Record DCOTAP value of last time
        newDcoTap = csCtl0Read & 0x01ff;          // Get DCOTAP value of this time
        dcoFreqTrim = (csCtl1Read & 0x0070) >> 4; // Get DCOFTRIM value

        if (newDcoTap < 256) // DCOTAP < 256
        {
            newDcoDelta = 256 - newDcoTap;                   // Delta value between DCPTAP and 256
            if ((oldDcoTap != 0xffff) && (oldDcoTap >= 256)) // DCOTAP cross 256
                endLoop = 1;                                 // Stop while loop
            else
            {
                dcoFreqTrim--;
                CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim << 4);
            }
        }
        else // DCOTAP >= 256
        {
            newDcoDelta = newDcoTap - 256; // Delta value between DCPTAP and 256
            if (oldDcoTap < 256)           // DCOTAP cross 256
                endLoop = 1;               // Stop while loop
            else
            {
                dcoFreqTrim++;
                CSCTL1 = (csCtl1Read & (~DCOFTRIM)) | (dcoFreqTrim << 4);
            }
        }

        if (newDcoDelta < bestDcoDelta) // Record DCOTAP closest to 256
        {
            csCtl0Copy = csCtl0Read;
            csCtl1Copy = csCtl1Read;
            bestDcoDelta = newDcoDelta;
        }

    } while (endLoop == 0); // Poll until endLoop == 1

    CSCTL0 = csCtl0Copy; // Reload locked DCOTAP
    CSCTL1 = csCtl1Copy; // Reload locked DCOFTRIM
    while (CSCTL7 & (FLLUNLOCK0 | FLLUNLOCK1))
        ; // Poll until FLL is locked
}

void initClock()
{
    __bis_SR_register(SCG0);                                   // disable FLL
    CSCTL3 |= SELREF__REFOCLK;                                 // Set REFO as FLL reference source
    CSCTL1 = DCOFTRIMEN_1 | DCOFTRIM0 | DCOFTRIM1 | DCORSEL_3; // DCOFTRIM=3, DCO Range = 8MHz
    CSCTL2 = FLLD_0 + 243;                                     // DCODIV = 8MHz
    __delay_cycles(3);
    __bic_SR_register(SCG0); // enable FLL
    Software_Trim();         // Software Trim to get the best DCOFTRIM value

    CSCTL4 = SELMS__DCOCLKDIV | SELA__REFOCLK; // set default REFO(~32768Hz) as ACLK source, ACLK = 32768Hz
                                               // default DCODIV as MCLK and SMCLK source
}

void setPins(void)
{
    // Configure all the GPIO Pins and Interfaces

    PM5CTL0 &= ~LOCKLPM5; // Disable the GPIO power-on default high-impedance mode
                          // to activate previously configured port settings

    initClock();

    // I2C Slave pins. From this interface the motheroard is calling
    P1SEL0 |= BIT2 | BIT3; // it is 1.2: UCB0_SDA and 1.3:UCB0_SCL

    // I2C Master pins. This interface is connecting sensor on I2C Board
    P3SEL0 |= BIT2 | BIT6; // I2C pins - it is 3.2: UCB1_SDA and 3.6:UCB1_SCL

    P1DIR |= BIT1;
    P1OUT |= BIT1;
    P1OUT &= ~BIT1;

    // I2C Adresse
    // configure pullups on Dipswitches Pins 0-4 which is Signals ADDR_B0 - ADDR_B4 on Board
    // ADDR_B0 --> P3.4
    // ADDR_B1 --> P2.3
    // ADDR_B2 --> P3.3
    // ADDR_B3 --> P3.0
    // ADDR_B4 --> P2.2
    // Port 2
    P2DIR &= ~BIT2;       // explicitly set as Input --> Bit 4
    P2DIR &= ~BIT3;       // explicitly set as Input --> Bit 1
    P2REN |= BIT2 | BIT3; // Enable Resistor (Pullup or down)
    P2OUT |= BIT2 | BIT3; // Pullup

    // Port 3
    P3DIR &= ~BIT0; // explicitly set as Input --> Bit 3
    P3DIR &= ~BIT3; // explicitly set as Input --> Bit 2
    P3DIR &= ~BIT4; // explicitly set as Input --> Bit 0

    P3REN |= BIT0 | BIT3 | BIT4; // Enable Resistor (Pullup or down)
    P3OUT |= BIT0 | BIT3 | BIT4; // Pullup

    // Interface Enable
    P3DIR |= BIT5;
    P3OUT |= BIT5;
    // Sensor V Low Side Switch
    P2DIR |= BIT7;
    P2OUT |= BIT7;

    return;
}

unsigned char getOwnI2CAddress(void)
{
    // read out GPIOS to determine own hardware configured address
    unsigned char address = 0x00;
    uint8_t address_int;
    uint8_t Addr_Bit_0, Addr_Bit_1, Addr_Bit_2, Addr_Bit_3, Addr_Bit_4;

    Addr_Bit_0 = ((~P3IN & BIT4) >> 4);
    Addr_Bit_1 = ((~P2IN & BIT3) >> 2);
    Addr_Bit_2 = ((~P3IN & BIT3) >> 1);
    Addr_Bit_3 = ((~P3IN & BIT0) << 3);
    Addr_Bit_4 = ((~P2IN & BIT2) << 2);

    // I2C Addresses (7 bit) from 0x00 up to 0x31
    address_int = Addr_Bit_0 | Addr_Bit_1 | Addr_Bit_2 | Addr_Bit_3 | Addr_Bit_4;
    address = address_int;
    return address;
}
