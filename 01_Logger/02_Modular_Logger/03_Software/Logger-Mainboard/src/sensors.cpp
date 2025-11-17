#include "sensors.h"
#include "logging.h"
#include "measurement_storage.h"

// =========================
// GLOBAL SENSOR DATA
// =========================
ConfigSensorInfo activeSensors[MAX_SENSORS];
uint8_t activeSensorCount = 0;

static const uint8_t SENSOR_RDY_RETRY_COUNT = 3;

static bool retrySensorReady(uint8_t sensorIndex, uint8_t attempts);

// =========================
// MAIN FUNCTIONS
// =========================

bool initializeConfiguredSensors()
{
    LOG_I("🔍 Initializing configured sensors...");

    activeSensorCount = 0;
    memset(activeSensors, 0, sizeof(activeSensors));

    // Wait for MSP430 startup time
    LOG_I("⏳ Waiting for MSP430 startup (2s)...");
    delay(2000);

    // Iterate through all configured parameters
    for (uint8_t i = 0; i < systemConfig.interfaceMcu.parameterCount && activeSensorCount < MAX_SENSORS; i++)
    {
        ConfigParameter &param = systemConfig.interfaceMcu.parameters[i];

        // Convert ID to bus address ("01" -> 1, "04" -> 4, etc.)
        uint8_t busAddress = param.id.toInt();

        if (busAddress == 0 && param.id != "00")
        {
            LOG_W("⚠ Invalid sensor ID: '%s' - skipped", param.id.c_str());
            continue;
        }

        LOG_I("🔍 Checking sensor ID=%s (address %d): %s",
              param.id.c_str(), busAddress, param.name.c_str());

        // Search for a sensor at this address
        I2C_SENSORS.beginTransmission(busAddress);
        I2C_SENSORS.write(CMD_PING);
        if (I2C_SENSORS.endTransmission() == 0)
        {
            // Sensor found - query details
            activeSensors[activeSensorCount].address = busAddress;
            activeSensors[activeSensorCount].version = getSensorVersion(busAddress);
            activeSensors[activeSensorCount].parameter = getSensorParameter(busAddress);
            activeSensors[activeSensorCount].externparameter = getExternSensorParameter(busAddress);
            activeSensors[activeSensorCount].active = true;
            activeSensors[activeSensorCount].configured = true;

            // Apply configuration data
            activeSensors[activeSensorCount].id = param.id;
            activeSensors[activeSensorCount].uuid = param.uuid;
            activeSensors[activeSensorCount].name = param.name;
            activeSensors[activeSensorCount].unit = param.unit;
            activeSensors[activeSensorCount].type = param.type;
            activeSensors[activeSensorCount].icon = param.icon;

            // Initialize measurement value
            activeSensors[activeSensorCount].currentValue = 0.0;
            activeSensors[activeSensorCount].valueValid = false;

            LOG_I("✓ Sensor found: Addr=%d, Name='%s', Type='%s', Param=%s",
                  busAddress, param.name.c_str(), param.type.c_str(),
                  getParameterName(activeSensors[activeSensorCount].parameter));

            activeSensorCount++;
        }
        else
        {
            LOG_E("Sensor ID=%s (address %d) not found: %s",
                  param.id.c_str(), busAddress, param.name.c_str());
        }

        delayMicroseconds(500);
    }

    LOG_I("✓ %d of %d configured sensors found",
          activeSensorCount, systemConfig.interfaceMcu.parameterCount);

    if (activeSensorCount == 0)
    {
        LOG_E("No sensors available!");
        return false;
    }

    // Activate and calibrate sensors
    return activateAndCalibrateSensors();
}

bool activateAndCalibrateSensors()
{
    LOG_I(" Activating and calibrating sensors...");

    // Activate all sensors
    for (uint8_t i = 0; i < activeSensorCount; i++)
    {
        uint8_t addr = activeSensors[i].address;
        String id = activeSensors[i].id;
        String name = activeSensors[i].name;

        LOG_I(" Activating sensor ID=%s (%s) @%d", id.c_str(), name.c_str(), addr);

        // Check sensor voltage
        uint8_t voltage = getSensorVoltage(addr);
        while (voltage == 0)
        {
            voltage = getSensorVoltage(addr);
            delay(10);
        }

        if (voltage == 3)
        {
            enable5V();
            LOG_I("Enable 5V");
        }

        if (voltage == 5)
        {
            enable12V();
            LOG_I("Enable 12V");
        }

        if (voltage == 7)
        {
            enable5V();
            enable12V();
            LOG_I("Enable 5V/12V");
        }

        // Wake the sensor and reset the MSP430
        wakeupSensor(addr);
        delay(50);
        msp430Reset(addr);
        delay(50);

        // Check connection
        bool connected = false;
        for (int j = 0; j < 5; j++)
        {
            if (checkSensorConnection(addr))
            {
                connected = true;
                break;
            }
            delay(100);
        }

        if (!connected)
        {
            LOG_E("MSP430 for sensor ID=%s not reachable", id.c_str());
            activeSensors[i].active = false;
            continue;
        }

        LOG_I("✓ Sensor ID=%s activated successfully", id.c_str());
    }

    // Wait for the longest wakeup time
    uint16_t longestWakeupTime = 0;
    for (uint8_t i = 0; i < activeSensorCount; i++)
    {
        if (!activeSensors[i].active)
            continue;
        uint16_t wakeupTime = getSensorWakeupTime(activeSensors[i].address);
        if (wakeupTime > longestWakeupTime)
        {
            longestWakeupTime = wakeupTime;
        }
    }

    if (longestWakeupTime > 0)
    {
        LOG_I("Waiting for sensor wakeup: %d ms", longestWakeupTime);
        delay(longestWakeupTime);
    }

    // Apply calibration
    applySensorCalibration();

    return true;
}

void applySensorCalibration()
{
    LOG_I("Applying sensor calibration...");

    for (uint8_t i = 0; i < activeSensorCount; i++)
    {
        if (!activeSensors[i].active)
            continue;

        uint8_t addr = activeSensors[i].address;
        String id = activeSensors[i].id;
        String name = activeSensors[i].name;

        LOG_I("Calibrating sensor ID=%s (%s)", id.c_str(), name.c_str());

        CalibrationCoeffs *calib = findCalibrationById(id);

        if (calib != nullptr && calib->coeffCount > 0)
        {
            LOG_I("  ✓ JSON calibration found: %d coefficients", calib->coeffCount);

            CalibCoeff coeffStruct = {0};

            // Copy available coefficients
            if (calib->coeffCount >= 1)
                coeffStruct.coeff1 = calib->coeffs[0];
            if (calib->coeffCount >= 2)
                coeffStruct.coeff2 = calib->coeffs[1];
            if (calib->coeffCount >= 3)
                coeffStruct.coeff3 = calib->coeffs[2];
            if (calib->coeffCount >= 4)
                coeffStruct.coeff4 = calib->coeffs[3];
            if (calib->coeffCount >= 5)
                coeffStruct.coeff5 = calib->coeffs[4];
            if (calib->coeffCount >= 6)
                coeffStruct.coeff6 = calib->coeffs[5];
            if (calib->coeffCount >= 7)
                coeffStruct.coeff7 = calib->coeffs[6];
            if (calib->coeffCount >= 8)
                coeffStruct.coeff8 = calib->coeffs[7];
            if (calib->coeffCount >= 9)
                coeffStruct.coeff9 = calib->coeffs[8];
            if (calib->coeffCount >= 10)
                coeffStruct.coeff10 = calib->coeffs[9];

            setAllCalibrationCoeffs(addr, coeffStruct);
            LOG_I("  ✓ Calibration transferred");
        }
        else
        {
            LOG_W("  ⚠ No calibration available for ID=%s", id.c_str());
        }

        delay(10);
    }

    LOG_I("✓ Sensor calibration complete");
}

void readAndCalibrateSensors()
{
    // Temperature compensation for oxygen sensor
    uint8_t tempAddr = findTemperatureSensorAddress();
    uint8_t oxygenAddr = findOxygenSensorAddress();

    if (tempAddr > 0 && oxygenAddr > 0)
    {
        // LOG_I("🌡️ Temperature compensation for oxygen sensor...");

        startSensorConversion(tempAddr);
        uint8_t rdy_status = getInterfaceRDY(tempAddr);
        if (rdy_status != 2)
        {
            uint32_t value1 = getSensorValue1(tempAddr);
            float tempValue = floatingPointConvert(value1);
            setSensorTempToInterface(oxygenAddr, tempValue);
            // LOG_I("  ✓ Temperature: %.2f°C sent to oxygen sensor", tempValue);
        }
        else
        {
            LOG_I("Temperature compensation for oxygen sensor not performed");
            Serial.println(rdy_status);
            Serial.println(tempAddr);
            Serial.println(oxygenAddr);
        }
    }

    // Start measurements for all sensors
    for (uint8_t i = 0; i < activeSensorCount; i++)
    {
        activeSensors[i].valueValid = false;

        if (!activeSensors[i].active)
            continue;

        uint8_t addr = activeSensors[i].address;
        startSensorConversion(addr);

        uint8_t rdy_status = getInterfaceRDY(addr);
        if (rdy_status == 2)
        {
            LOG_W("⚠ Sensor ID=%s not ready", activeSensors[i].id.c_str());
            activateAndCalibrateSensors();

            //! if (retrySensorReady(i, SENSOR_RDY_RETRY_COUNT))
            //! {
            //!     activeSensors[i].valueValid = true;
            //! }
            //! else
            //! {
            //!     LOG_E("Sensor ID=%s not ready after %d attempts", activeSensors[i].id.c_str(), SENSOR_RDY_RETRY_COUNT);
            //! }
        }
        else
        {
            activeSensors[i].valueValid = true;
        }
    }

    // Briefly wait for conversion
    //! delay(10);

    // Read all measurement values
    for (uint8_t i = 0; i < activeSensorCount; i++)
    {
        if (!activeSensors[i].active)
        {
            continue;
        }

        // Special handling for oxygen sensor
        if (activeSensors[i].address == oxygenAddr && oxygenAddr > 0)
        {
            // Check if temperature sensor is available and ready
            bool tempSensorOK = false;
            if (tempAddr > 0)
            {
                // Find temperature sensor in the array
                for (uint8_t j = 0; j < activeSensorCount; j++)
                {
                    if (activeSensors[j].address == tempAddr && activeSensors[j].valueValid)
                    {
                        tempSensorOK = true;
                        break;
                    }
                }
            }

            if (!tempSensorOK)
            {
                LOG_W("⚠ Oxygen sensor ID=%s: temperature sensor not ready → value = -1", activeSensors[i].id.c_str());
                activeSensors[i].currentValue = -1;
                activeSensors[i].valueValid = true;
                continue;
            }
        }

        // Regular sensor not ready
        if (!activeSensors[i].valueValid)
        {
            LOG_W("⚠ Sensor ID=%s not ready → value = -1", activeSensors[i].id.c_str());
            activeSensors[i].currentValue = -1;
            activeSensors[i].valueValid = true;
            continue;
        }

        // Read the standard measurement value
        uint8_t addr = activeSensors[i].address;
        uint32_t value1 = getSensorValue1(addr);
        activeSensors[i].currentValue = floatingPointConvert(value1);
        activeSensors[i].valueValid = true;
    }
}

static bool retrySensorReady(uint8_t sensorIndex, uint8_t attempts)
{
    if (attempts == 0 || sensorIndex >= activeSensorCount)
    {
        return false;
    }

    ConfigSensorInfo &sensor = activeSensors[sensorIndex];

    if (!sensor.active)
    {
        return false;
    }

    uint8_t address = sensor.address;

    for (uint8_t attempt = 1; attempt <= attempts; attempt++)
    {
        LOG_W("↻ Sensor ID=%s: attempting reinitialization (%d/%d)", sensor.id.c_str(), attempt, attempts);

        wakeupSensor(address);
        msp430Reset(address);
        delay(2000);

        if (!checkSensorConnection(address))
        {
            LOG_W("  ⚠ Sensor ID=%s not reachable after reset", sensor.id.c_str());
            continue;
        }

        startSensorConversion(address);
        uint8_t rdy_status = getInterfaceRDY(address);

        if (rdy_status != 2)
        {
            LOG_I("  ✓ Sensor ID=%s ready again", sensor.id.c_str());
            return true;
        }
    }

    return false;
}

void printSensorData()
{
    // LOG_I("======== SENSOR MEASUREMENTS ========");

    DateTime now = rtc.now();
    LOG_I("Time: %04d-%02d-%02d %02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

    for (uint8_t i = 0; i < activeSensorCount; i++)
    {
        if (!activeSensors[i].active)
            continue;

        if (activeSensors[i].valueValid)
        {
            LOG_I("ID=%s %.4f",
                  activeSensors[i].id.c_str(),
                  activeSensors[i].currentValue);
        }
        else
        {
            LOG_E("ID=%s (%s): measurement error",
                  activeSensors[i].id.c_str(),
                  activeSensors[i].name.c_str());
        }
    }

    // LOG_I("=====================================");
}

// =========================
// I2C SENSOR FUNCTIONS
// =========================

bool checkSensorConnection(uint8_t address)
{
    I2C_SENSORS.beginTransmission(address);
    I2C_SENSORS.write(CMD_PING);
    return (I2C_SENSORS.endTransmission() == 0);
}

uint8_t getSensorVersion(uint8_t address)
{
    I2C_SENSORS.beginTransmission(address);
    I2C_SENSORS.write(CMD_GETVER);
    if (I2C_SENSORS.endTransmission() != 0)
        return 0;

    I2C_SENSORS.requestFrom(address, (uint8_t)4);
    if (I2C_SENSORS.available() >= 4)
    {
        I2C_SENSORS.read(); // Skip first 3 bytes
        I2C_SENSORS.read();
        I2C_SENSORS.read();
        return I2C_SENSORS.read(); // Return 4th byte (sensor type ID)
    }
    return 0;
}

uint8_t getSensorParameter(uint8_t address)
{
    I2C_SENSORS.beginTransmission(address);
    I2C_SENSORS.write(CMD_GET_PARAMETER);
    if (I2C_SENSORS.endTransmission() != 0)
        return 0;

    I2C_SENSORS.requestFrom(address, (uint8_t)1);
    return I2C_SENSORS.available() ? I2C_SENSORS.read() : 0;
}

uint8_t getExternSensorParameter(uint8_t address)
{
    I2C_SENSORS.beginTransmission(address);
    I2C_SENSORS.write(CMD_GET_EXTERNPARAMETER);
    if (I2C_SENSORS.endTransmission() != 0)
        return 0;

    I2C_SENSORS.requestFrom(address, (uint8_t)1);
    return I2C_SENSORS.available() ? I2C_SENSORS.read() : 0;
}

void startSensorConversion(uint8_t address)
{
    I2C_SENSORS.beginTransmission(address);
    I2C_SENSORS.write(CMD_CONVERT);
    I2C_SENSORS.endTransmission();
}

uint8_t getInterfaceRDY(uint8_t address)
{
    uint8_t count = 0;

    while (count < 100)
    {
        I2C_SENSORS.beginTransmission(address);
        I2C_SENSORS.write(CMD_GET_RDY);
        if (I2C_SENSORS.endTransmission() != 0)
            return 2;

        I2C_SENSORS.requestFrom(address, (uint8_t)1);
        if (I2C_SENSORS.available())
        {
            uint8_t rdy_status = I2C_SENSORS.read();

            if (rdy_status == 1)
            {
                return 1; // Sensor ready
            }
            else if (rdy_status == 2)
            {
                return 2; // Sensor not ready
            }
            else if (rdy_status == 0)
            {
                delay(20);
                count++;
                // Continue loop
            }
            else
            {
                return 2; // Unknown status
            }
        }
        else
        {
            return 2; // No response
        }
    }

    return 2; // Timeout
}

int64_t getSensorValue1(uint8_t address)
{
    uint8_t buffer[8];
    I2C_SENSORS.beginTransmission(address);
    I2C_SENSORS.write(CMD_GETVALUE1);
    if (I2C_SENSORS.endTransmission() != 0)
        return 0;

    I2C_SENSORS.requestFrom(address, (uint8_t)8);
    if (I2C_SENSORS.available() == 8)
    {
        for (int i = 0; i < 8; i++)
        {
            buffer[i] = I2C_SENSORS.read();
        }

        int64_t valueRaw = ((int64_t)buffer[7]);
        valueRaw |= (((int64_t)buffer[6]) << 8);
        valueRaw |= (((int64_t)buffer[5]) << 16);
        valueRaw |= (((int64_t)buffer[4]) << 24);
        valueRaw |= (((int64_t)buffer[3]) << 32);
        valueRaw |= (((int64_t)buffer[2]) << 40);
        valueRaw |= (((int64_t)buffer[1]) << 48);
        valueRaw |= (((int64_t)buffer[0]) << 56);

        return valueRaw;
    }
    return 0;
}

float floatingPointConvert(uint32_t decimalValue)
{
    union
    {
        uint32_t i;
        float f;
    } converter;

    converter.i = decimalValue;
    return converter.f;
}

void wakeupSensor(uint8_t address)
{
    I2C_SENSORS.beginTransmission(address);
    I2C_SENSORS.write(CMD_SENSOR_WAKEUP);
    I2C_SENSORS.endTransmission();
    delay(10);
}

void sleepSensor(uint8_t address)
{
    I2C_SENSORS.beginTransmission(address);
    I2C_SENSORS.write(CMD_SENSOR_SLEEP);
    I2C_SENSORS.endTransmission();
    delay(10);
}

void msp430Reset(uint8_t address)
{
    I2C_SENSORS.beginTransmission(address);
    I2C_SENSORS.write(CMD_SOFTWARE_RESET);
    I2C_SENSORS.endTransmission();
    delay(10);
}

uint8_t getSensorVoltage(uint8_t address)
{
    I2C_SENSORS.beginTransmission(address);
    I2C_SENSORS.write(CMD_GET_SENSORVOLTAGE);
    if (I2C_SENSORS.endTransmission() != 0)
        return 0;

    I2C_SENSORS.requestFrom(address, (uint8_t)1);
    return I2C_SENSORS.available() ? I2C_SENSORS.read() : 0;
}

uint16_t getSensorWakeupTime(uint8_t address)
{
    uint8_t buffer[2];

    I2C_SENSORS.beginTransmission(address);
    I2C_SENSORS.write(CMD_GET_SENSOR_WAKEUP_TIME);
    if (I2C_SENSORS.endTransmission() != 0)
        return 0;

    I2C_SENSORS.requestFrom(address, (uint8_t)2);
    if (I2C_SENSORS.available() == 2)
    {
        buffer[0] = I2C_SENSORS.read();
        buffer[1] = I2C_SENSORS.read();
        return (buffer[0] << 8) | buffer[1]; // Big Endian
    }
    return 0;
}

void setSensorTempToInterface(uint8_t address, float temperature)
{
    union
    {
        float float_variable;
        unsigned char temp_array[4];
    } u;

    u.float_variable = temperature;

    uint8_t data[4];
    data[0] = u.temp_array[3]; // Big Endian
    data[1] = u.temp_array[2];
    data[2] = u.temp_array[1];
    data[3] = u.temp_array[0];

    I2C_SENSORS.beginTransmission(address);
    I2C_SENSORS.write(CMD_SET_TEMP);
    I2C_SENSORS.write(data, 4);
    I2C_SENSORS.endTransmission();
    delay(10);
}

// =========================
// CALIBRATION
// =========================

void setCalibrationCoeff(uint8_t address, uint8_t index, float value)
{
    union
    {
        float float_variable;
        unsigned char temp_array[4];
    } u;

    u.float_variable = value;

    uint8_t data[5];
    data[0] = index;           // Index (1-10)
    data[1] = u.temp_array[3]; // Big Endian
    data[2] = u.temp_array[2];
    data[3] = u.temp_array[1];
    data[4] = u.temp_array[0];

    I2C_SENSORS.beginTransmission(address);
    I2C_SENSORS.write(CMD_SET_CALIB);
    I2C_SENSORS.write(data, 5);
    I2C_SENSORS.endTransmission();
}

void setAllCalibrationCoeffs(uint8_t address, const CalibCoeff &calib)
{
    delay(10); // Important for sensor stability
    setCalibrationCoeff(address, 1, calib.coeff1);
    setCalibrationCoeff(address, 2, calib.coeff2);
    setCalibrationCoeff(address, 3, calib.coeff3);
    setCalibrationCoeff(address, 4, calib.coeff4);
    setCalibrationCoeff(address, 5, calib.coeff5);
    setCalibrationCoeff(address, 6, calib.coeff6);
    setCalibrationCoeff(address, 7, calib.coeff7);
    setCalibrationCoeff(address, 8, calib.coeff8);
    setCalibrationCoeff(address, 9, calib.coeff9);
    setCalibrationCoeff(address, 10, calib.coeff10);
}

// =========================
// HELPER FUNCTIONS
// =========================

ConfigSensorInfo *findSensorByAddress(uint8_t address)
{
    for (uint8_t i = 0; i < activeSensorCount; i++)
    {
        if (activeSensors[i].address == address && activeSensors[i].active)
        {
            return &activeSensors[i];
        }
    }
    return nullptr;
}

uint8_t findTemperatureSensorAddress()
{
    for (uint8_t i = 0; i < activeSensorCount; i++)
    {
        if (activeSensors[i].active &&
            (activeSensors[i].type == "temperature" || activeSensors[i].parameter == PARAM_TEMPERATURE))
        {
            return activeSensors[i].address;
        }
    }
    return 0;
}

uint8_t findOxygenSensorAddress()
{
    for (uint8_t i = 0; i < activeSensorCount; i++)
    {
        if (activeSensors[i].active &&
            (activeSensors[i].type == "molar_doxy" || activeSensors[i].parameter == PARAM_OXYGEN))
        {
            return activeSensors[i].address;
        }
    }
    return 0;
}

const char *getParameterName(uint8_t parameter)
{
    switch (parameter)
    {
    case PARAM_TEMPERATURE:
        return "Temperature";
    case PARAM_PRESSURE:
        return "Pressure";
    case PARAM_OXYGEN:
        return "Oxygen";
    case PARAM_CONDUCTIVITY:
        return "Conductivity";
    case PARAM_ANALOG:
        return "Analog";
    case PARAM_NOEXTERNPARAMETER:
        return "No Extern Parameter";
    default:
        return "Unknown";
    }
}

/**
 * @brief Enables 12V power supply.
 */
void enable12V()
{
    pinMode(GPIO_NUM_11, OUTPUT);
    gpio_hold_dis(GPIO_NUM_11);
    digitalWrite(GPIO_NUM_11, HIGH);
    delay(10);
    gpio_hold_en(GPIO_NUM_11);
}

/**
 * @brief Enables 5V power supply.
 */
void enable5V()
{
    pinMode(GPIO_NUM_12, OUTPUT);
    gpio_hold_dis(GPIO_NUM_12);
    digitalWrite(GPIO_NUM_12, HIGH);
    delay(10);
    gpio_hold_en(GPIO_NUM_12);
}

/**
 * @brief Disables 12V power supply.
 */
void disable12V()
{
    pinMode(GPIO_NUM_11, OUTPUT);
    gpio_hold_dis(GPIO_NUM_11);
    digitalWrite(GPIO_NUM_11, LOW);
    delay(500);
    gpio_hold_en(GPIO_NUM_11);
}

/**
 * @brief Disables 5V power supply.
 */
void disable5V()
{
    pinMode(GPIO_NUM_12, OUTPUT);
    gpio_hold_dis(GPIO_NUM_12);
    digitalWrite(GPIO_NUM_12, LOW);
    delay(500);
    gpio_hold_en(GPIO_NUM_12);
}