/*
 * SPDX-FileCopyrightText: (C) 2024 Frederik Furkert, Thuenen-Institute of Baltic Sea Fisheries
 *
 * SPDX-License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Interface to the oxygen sensor "picoO2sub" from the manufacturer PyroScience GmbH
 */
 
#include <Arduino.h>
#include <HardwareSerial.h>
#include <driver/uart.h>
#include <string> 

#ifndef PYROO2_H
#define PYROO2_H

#pragma once

class pyroO2
{
    public:
        pyroO2();

        // Init uart com
            /* init Uart driver in order to communicate with oxygen sensor picoO2sub.
            Calls this->setDeviceID */
            void initUart (uart_port_t uart_num_in, int RX_in, int TX_in);

        // Info
            char* getInfo();
        
        /* requests device ID from sensor and saves it in char* device ID. Is called by initUart()*/
            void setDeviceID();

        // Power management
            /* Send whole module to deep sleep to save energy*/
            void enterDeepSleep();
            /* Wake up from deep sleep. Fuction needs to wait 250 ms for sensor to wake up*/
            void exitDeepSleep();
            /* Power down the sensor circuits. Module stays awake, so less energy saving than deep sleep
            sensor cuircuit is powered up again, if a measurement request is received (MEA).*/
            void powerDown();
            /* power up sensor circuits. Function needs to wait 250 ms for sensor to power up*/
            void powerUp();

        // Measurement
            /* Set sample salinity. Salinity value given by external conductivity sensor.
            SampleSal shall be given in g/l. 
            Value only required for use of output unit: umolar (mg/l)*/
            void writeSampleSal (float sampleSal); 
            /* Set sample temperature.
            Sample Temp shall be given in °C.
            Hand over no argument (SampleTemp = -299.999) to set sensor to use own temperature sensor inside sensor case.
            Hand over SampleTemp = -300 to set sensor to use own external temp sensor Pt100 (usually not connected in HyFiVe logger). */
            void writeSampleTemp (float sampleTemp); 
            /* Triggers standard measurment. */
            void measure();             
            /* reads Uart buffer and tries to interprete the message as measurement (recognised by segment "MEA") */
            void read();    
            /* Read multiple register, s. picoO2sub manual regarding command RMR
            T Register block number:
            T=0: Settings registers T=1: Calibration registers T=3: Results registers T=4: AnalogOutput registers
            R Start register number (R=0 for starting with the first register)
            N Number of registers to read. */            
            void readRegister(int T, int R, int N);


        // accessing data
            const char* getMessageReceived();
            const char* getErrorMessages ();
            float getOxygenUMolar ();
            float getOxygenMBar ();
            float getHumidityInCase ();
            float getTempCase ();
            float getTempSample ();
            const char* getDeviceID();

        // only for validation purpose (set sensor response manually to check processing of data)
            void setRawMeasurement (const char* messIn); 


    private:
        // Uart com
            // Uart init parameter
            uart_port_t uart_num;
            int RX_PIN;
            int TX_PIN;
            
            /* Receive a message via Uart from sensor.
            Reads out Uart buffer and writes to char* messageReceived*/
            void receive ();
            /* Send a message via UART to sensor*/
            void send (const char* messToSend);


        // Analyse measurement data
            /* Checks if char* measurementRaw contains a measurment (recognised by segment "MEA"). 
            If yes, relevant measurment variables are set accordingly.*/
            void interpreteRawMeas();  
            /* Translates bits of int errorint into corresponding error messages, saved in char* errorMessages*/  
            char* interpreteErrInt(int error);


        // c_strings
            char errorMessages [520];
            char measurementRaw [256];
            char messageReceived [256];
            char command [50];
            char deviceID [20];

        // messurement results
            float oxygenUMolar;
            float oxygenMBar;
            float humidityInCase;
            float tempSample;
            float tempCase;        

};

#endif