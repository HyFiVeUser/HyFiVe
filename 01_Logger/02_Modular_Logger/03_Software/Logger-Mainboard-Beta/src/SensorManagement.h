/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Sensor management and data collection
 */

#ifndef ALLEFUNKTIONEN_H
#define ALLEFUNKTIONEN_H

// Initialization

void initializeLogger();

// Water detection

bool checkWetSensorStatus();

// Sensor configuration and detection

void detectConnectedSensorDevices();
void detectDryWetCastSensors();
void detectConnecteOxygenSensor(uint8_t sensorNumber);

// Measurements

void updateSensorMeasurements();
void performSensorMeasurement(int sensorNumber);
void performInitialMeasurement();

// Energy management

void setRequiredVoltage(bool enable); // Activates or deactivates the required voltage for sensors

// Data processing and storage

void writeMeasurementDataToFile();
void createConfigHeader();

// Deep sleep and energy-saving mode

float calculateShortestSensorWaitTime(); // Calculates the shortest waiting time until the next measurement

// Underwater and above water mode

void performUnderWaterOperations();

// Sensor communication and configuration

String interfaceVersion(int bus_address);
void interfaceSensorVoltage(int bus_address);
String interfaceParameterName(int bus_address);
void setSensorTempToInterface(uint8_t bus_address, float temp);
void setSensorCalibToInterface(uint8_t bus_address, uint8_t index, float calib);
void interfaceSleep();
void interfaceWakeup();
void sensorCalibToInterface();

// Error handling

void sensorAvailability();

#endif