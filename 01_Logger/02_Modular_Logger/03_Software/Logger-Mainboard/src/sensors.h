#ifndef SYSTEM_SENSORS_H
#define SYSTEM_SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "init.h"

// =========================
// SENSOR COMMANDS
// =========================

#define CMD_GETVER 0x00
#define CMD_PING 0xAA
#define CMD_GETVALUE1 0x03
#define CMD_CONVERT 0x05
#define CMD_GET_PARAMETER 0x15
#define CMD_GET_EXTERNPARAMETER 0x22
#define CMD_GET_RDY 0x16
#define CMD_SENSOR_WAKEUP 0x07
#define CMD_SOFTWARE_RESET 0x21
#define CMD_SENSOR_SLEEP 0x06
#define CMD_SET_CALIB 0x17
#define CMD_GET_SENSORVOLTAGE 0x09
#define CMD_SET_TEMP 0x08
#define CMD_GET_FW_VERSION 0x20
#define CMD_GET_SENSOR_WAKEUP_TIME 0x19

// =========================
// SENSOR PARAMETER TYPES
// =========================

enum SensorParameter {
    PARAM_UNKNOWN = 0,
    PARAM_TEMPERATURE = 1,
    PARAM_PRESSURE = 2,
    PARAM_OXYGEN = 3,
    PARAM_CONDUCTIVITY = 4,
    PARAM_ANALOG = 5,
    PARAM_NOEXTERNPARAMETER = 255,
};

// =========================
// SENSOR STRUCTURES
// =========================

// Extended sensor info with config data
struct ConfigSensorInfo {
    uint8_t address;           // I2C bus address
    uint8_t version;           // Hardware version
    uint8_t parameter;         // Sensor parameter type
    uint8_t externparameter;   // External parameter type
    bool active;               // Sensor active/available
    bool configured;           // Defined in config.json
    
    // Config data from JSON
    String id;                 // ID from config.json ("01", "02", etc.)
    String uuid;               // Name from config.json
    String name;               // Name from config.json
    String unit;               // Unit from config.json
    String type;               // Type from config.json
    String icon;               // Icon from config.json
    
    // Current measurement value
    float currentValue;        // Last measurement value
    bool valueValid;           // Measurement valid?
};

// Calibration coefficients in I2C format
struct CalibCoeff {
    float coeff1, coeff2, coeff3, coeff4, coeff5;
    float coeff6, coeff7, coeff8, coeff9, coeff10;
};

// =========================
// GLOBAL SENSOR DATA
// =========================

#define MAX_SENSORS 32
extern ConfigSensorInfo activeSensors[MAX_SENSORS];
extern uint8_t activeSensorCount;

// =========================
// MAIN FUNCTIONS
// =========================

/**
 * @brief Initializes the I2C bus and scans all configured sensors.
 * @return true if at least one sensor was found and registered, otherwise false.
 */
bool initializeConfiguredSensors();

/**
 * @brief Activates and calibrates all previously discovered sensors.
 * @return true if every active sensor was prepared, otherwise false.
 */
bool activateAndCalibrateSensors();

/**
 * @brief Applies existing calibration data to all active sensors.
 * @return void
 */
void applySensorCalibration();

/**
 * @brief Reads all active sensors and calibrates the measurements if needed.
 * @return void
 */
void readAndCalibrateSensors();

/**
 * @brief Prints all sensor measurements in a formatted way over the serial interface.
 * @return void
 */
void printSensorData();

// =========================
// SENSOR I2C FUNCTIONS
// =========================

/**
 * @brief Checks whether a sensor is reachable at the specified I2C address.
 * @param address I2C address of the sensor.
 * @return true if the sensor responds, otherwise false.
 */
bool checkSensorConnection(uint8_t address);

/**
 * @brief Reads the firmware version of a sensor.
 * @param address I2C address of the sensor.
 * @return Read firmware version number.
 */
uint8_t getSensorVersion(uint8_t address);

/**
 * @brief Determines the parameter type of the sensor.
 * @param address I2C address of the sensor.
 * @return Parameter code according to the sensor protocol.
 */
uint8_t getSensorParameter(uint8_t address);

/**
 * @brief Determines the external parameter type of the sensor.
 * @param address I2C address of the sensor.
 * @return External parameter code according to the sensor protocol.
 */
uint8_t getExternSensorParameter(uint8_t address);

/**
 * @brief Starts a new measurement on the sensor.
 * @param address I2C address of the sensor.
 * @return void
 */
void startSensorConversion(uint8_t address);

/**
 * @brief Checks whether the sensor has completed the requested measurement.
 * @param address I2C address of the sensor.
 * @return Status code from the sensor.
 */
uint8_t getInterfaceRDY(uint8_t address);

/**
 * @brief Reads the current measurement value from the sensor.
 * @param address I2C address of the sensor.
 * @return Raw measurement value provided by the sensor.
 */
int64_t getSensorValue1(uint8_t address);

/**
 * @brief Converts a 32-bit raw value into an IEEE-754 floating-point value.
 * @param decimalValue Raw measurement in 32-bit format.
 * @return Converted measurement as float.
 */
float floatingPointConvert(uint32_t decimalValue);

/**
 * @brief Wakes the sensor from sleep mode.
 * @param address I2C address of the sensor.
 * @return void
 */
void wakeupSensor(uint8_t address);

/**
 * @brief Puts the sensor into sleep mode.
 * @param address I2C address of the sensor.
 * @return void
 */
void sleepSensor(uint8_t address);

/**
 * @brief Performs a software reset of the sensor microcontroller.
 * @param address I2C address of the sensor.
 * @return void
 */
void msp430Reset(uint8_t address);

/**
 * @brief Reads the supply voltage of the sensor.
 * @param address I2C address of the sensor.
 * @return Reported voltage in volts.
 */
uint8_t getSensorVoltage(uint8_t address);

/**
 * @brief Reads the wake-up time of the sensor.
 * @param address I2C address of the sensor.
 * @return Wake-up time in milliseconds.
 */
uint16_t getSensorWakeupTime(uint8_t address);

/**
 * @brief Sends a temperature value for compensation to the sensor.
 * @param address I2C address of the sensor.
 * @param temperature Temperature value in degrees Celsius.
 * @return void
 */
void setSensorTempToInterface(uint8_t address, float temperature);

// =========================
// CALIBRATION
// =========================

/**
 * @brief Writes a single calibration coefficient to the sensor.
 * @param address I2C address of the sensor.
 * @param index Index of the coefficient (1-10).
 * @param value Coefficient value to transfer.
 * @return void
 */
void setCalibrationCoeff(uint8_t address, uint8_t index, float value);

/**
 * @brief Transfers all calibration coefficients to the sensor in one step.
 * @param address I2C address of the sensor.
 * @param calib Structure with up to ten calibration coefficients.
 * @return void
 */
void setAllCalibrationCoeffs(uint8_t address, const CalibCoeff& calib);

// =========================
// HELPER FUNCTIONS
// =========================

/**
 * @brief Searches for a sensor by its I2C address in the active sensor list.
 * @param address I2C address of the desired sensor.
 * @return Pointer to the sensor or nullptr if no match was found.
 */
ConfigSensorInfo* findSensorByAddress(uint8_t address);

/**
 * @brief Determines the I2C address of the configured temperature sensor.
 * @return I2C address of the temperature sensor or 0 if none is available.
 */
uint8_t findTemperatureSensorAddress();

/**
 * @brief Determines the I2C address of the configured oxygen sensor.
 * @return I2C address of the oxygen sensor or 0 if none is available.
 */
uint8_t findOxygenSensorAddress();

/**
 * @brief Converts a sensor parameter code into a descriptive string.
 * @param parameter Parameter code defined in the sensor specification.
 * @return Readable name of the parameter.
 */
const char* getParameterName(uint8_t parameter);


/**
 * @brief Enables 12V power supply.
 */
void enable12V();

/**
 * @brief Enables 5V power supply.
 */
void enable5V();

/**
 * @brief Disables 12V power supply.
 */
void disable12V();

/**
 * @brief Disables 5V power supply.
 */
void disable5V();

#endif
