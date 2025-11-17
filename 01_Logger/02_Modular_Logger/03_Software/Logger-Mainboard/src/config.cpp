#include "config.h"
#include "init.h"
#include "logging.h"
#include "led.h"

// =========================
// GLOBAL CONFIGURATION
// =========================
Config systemConfig;
CalibrationData calibrationData;

// =========================
// LOAD CONFIGURATION
// =========================

void sensorPrepDurationTime()
{
    uint32_t sensorPrepDuration = systemConfig.interfaceMcu.sensorPrepMs;
    if (sensorPrepDuration > 0)
    {
        ledSetSensorPrepActive(true);
        ledSignalSensorPreparation(sensorPrepDuration);

        uint32_t start = millis();
        while ((millis() - start) < sensorPrepDuration)
        {
            delay(50);
        }

        ledSetSensorPrepActive(false);
    }
}

bool loadConfiguration()
{
    LOG_I("📂 Loading configuration files...");

    bool configurationLoadedSuccessfully = loadConfigJSON();
    bool calibrationLoadedSuccessfully = loadCalibrationJSON();

    if (configurationLoadedSuccessfully)
    {
        // Apply measurement cycle from configuration
        measurementCycle = systemConfig.interfaceMcu.sampleIntervalMs / 1000;
        if (measurementCycle < 1)
            measurementCycle = 5; // Minimum 1 second

        LOG_I("✓ Measurement cycle: %llu seconds", measurementCycle);
        printConfiguration();
    }

    if (calibrationLoadedSuccessfully)
    {
        printCalibration();
    }

    return configurationLoadedSuccessfully; // Calibration is optional
}

bool loadConfigJSON()
{
    if (!SD.exists(CONFIG_FILE))
    {
        LOG_W("⚠ config.json not found");
        return false;
    }

    File file = SD.open(CONFIG_FILE);
    if (!file)
    {
        LOG_E("config.json cannot be opened");
        return false;
    }

    if (file.size() == 0)
    {
        LOG_E("config.json is empty");
        file.close();
        return false;
    }

    // Parse JSON (6KB buffer)
    DynamicJsonDocument configurationJson(6144);
    DeserializationError error = deserializeJson(configurationJson, file);
    file.close();

    if (error)
    {
        LOG_E("JSON Parse Error: %s", error.c_str());
        return false;
    }

    // MainMCU configuration
    if (configurationJson.containsKey("mainMcu"))
    {
        JsonObject mainControllerConfig = configurationJson["mainMcu"];

        systemConfig.mainMcu.datanodeAddress = mainControllerConfig["datanodeAddress"].as<String>();
        systemConfig.mainMcu.wifiSsid = mainControllerConfig["wifiSsid"].as<String>();
        systemConfig.mainMcu.wifiPassword = mainControllerConfig["wifiPassword"].as<String>();
        systemConfig.mainMcu.itemUuid = mainControllerConfig["itemUuid"].as<String>();
        systemConfig.mainMcu.itemToken = mainControllerConfig["itemToken"].as<String>();
        systemConfig.mainMcu.bluetoothName = mainControllerConfig["bluetoothName"].as<String>();
        systemConfig.mainMcu.poweroffThresholdMs = mainControllerConfig["poweroffThresholdMs"] | 2000;
        systemConfig.mainMcu.poweroffEnable = mainControllerConfig["poweroffEnable"] | false;
        systemConfig.mainMcu.measurementStart = mainControllerConfig["measurementStart"] | -1;
        systemConfig.mainMcu.measurementStop = mainControllerConfig["measurementStop"] | -1;
    }

        // InterfaceMCU configuration
    if (configurationJson.containsKey("interfaceMcu"))
    {
        JsonObject interfaceControllerConfig = configurationJson["interfaceMcu"];

        // Sensor Interfaces
        JsonArray sensorInterfaceList = interfaceControllerConfig["sensorInterfaces"];
        for (size_t i = 0; i < sensorInterfaceList.size() && i < 2; i++)
        {
            systemConfig.interfaceMcu.sensorInterfaces[i] = sensorInterfaceList[i].as<String>();
        }

        systemConfig.interfaceMcu.i2cFrequency = interfaceControllerConfig["i2cFrequency"] | 100000;
        systemConfig.interfaceMcu.sensorBaudrate = interfaceControllerConfig["sensorBaudrate"] | 19200;
        systemConfig.interfaceMcu.sensorVoltage = interfaceControllerConfig["sensorVoltage"] | 3.3;
        systemConfig.interfaceMcu.sampleIntervalMs = interfaceControllerConfig["sampleIntervalMs"] | 10000;
        systemConfig.interfaceMcu.sensorPrepMs = interfaceControllerConfig["sensorPrepMs"] | 0;

        // Python files
        JsonArray pythonFileList = interfaceControllerConfig["pythonFileNames"];
        for (size_t i = 0; i < pythonFileList.size() && i < 5; i++)
        {
            systemConfig.interfaceMcu.pythonFileNames[i] = pythonFileList[i].as<String>();
        }

        // Parameters (sensors)
        JsonArray sensorParameterArray = interfaceControllerConfig["parameters"];
        systemConfig.interfaceMcu.parameterCount = 0;

        for (size_t i = 0; i < sensorParameterArray.size() && i < MAX_PARAMETERS; i++)
        {
            JsonObject sensorParameterJson = sensorParameterArray[i];

            systemConfig.interfaceMcu.parameters[i].id = sensorParameterJson["id"].as<String>();
            systemConfig.interfaceMcu.parameters[i].uuid = sensorParameterJson["uuid"].as<String>();
            systemConfig.interfaceMcu.parameters[i].name = sensorParameterJson["name"].as<String>();
            systemConfig.interfaceMcu.parameters[i].unit = sensorParameterJson["unit"].as<String>();
            systemConfig.interfaceMcu.parameters[i].type = sensorParameterJson["type"].as<String>();
            systemConfig.interfaceMcu.parameters[i].icon = sensorParameterJson["icon"].as<String>();

            systemConfig.interfaceMcu.parameterCount++;
        }
    }

    LOG_I("✓ config.json loaded (%d parameters)",
          systemConfig.interfaceMcu.parameterCount);
    return true;
}

bool loadCalibrationJSON()
{
    if (!SD.exists(CALIB_FILE))
    {
        LOG_W("⚠ calib_coeff.json not found");
        return false;
    }

    File file = SD.open(CALIB_FILE);
    if (!file)
    {
        LOG_E("calib_coeff.json cannot be opened");
        return false;
    }

    if (file.size() == 0)
    {
        LOG_E("calib_coeff.json is empty");
        file.close();
        return false;
    }

    // Parse JSON (4KB buffer)
    DynamicJsonDocument calibrationJsonDocument(4096);
    DeserializationError error = deserializeJson(calibrationJsonDocument, file);
    file.close();

    if (error)
    {
        LOG_E("Calibration JSON Parse Error: %s", error.c_str());
        return false;
    }

    // InterfaceMCU calibration
    if (calibrationJsonDocument.containsKey("interfaceMcu"))
    {
        JsonObject interfaceControllerCalibration = calibrationJsonDocument["interfaceMcu"];

        if (interfaceControllerCalibration.containsKey("parameters"))
        {
            JsonArray calibrationParameterArray = interfaceControllerCalibration["parameters"];
            calibrationData.parameterCount = 0;

            for (size_t i = 0; i < calibrationParameterArray.size() && i < MAX_PARAMETERS; i++)
            {
                JsonObject calibrationParameterJson = calibrationParameterArray[i];

                calibrationData.parameters[i].id = calibrationParameterJson["id"].as<String>();
                calibrationData.parameters[i].name = calibrationParameterJson["name"].as<String>();
                calibrationData.parameters[i].coeffCount = 0;

                // Calibration coefficients
                if (calibrationParameterJson.containsKey("calib_coeff"))
                {
                    JsonObject coefficientDefinition = calibrationParameterJson["calib_coeff"];

                    // Iterate through all coefficients
                    for (JsonPair coefficientEntry : coefficientDefinition)
                    {
                        String coefficientIndexKey = coefficientEntry.key().c_str();
                        int coefficientIndex = coefficientIndexKey.toInt() - 1; // "1" -> Index 0, "2" -> Index 1

                        if (coefficientIndex >= 0 && coefficientIndex < 10)
                        {
                            calibrationData.parameters[i].coeffs[coefficientIndex] = coefficientEntry.value().as<float>();
                            if (coefficientIndex + 1 > calibrationData.parameters[i].coeffCount)
                            {
                                calibrationData.parameters[i].coeffCount = coefficientIndex + 1;
                            }
                        }
                    }
                }

                calibrationData.parameterCount++;
            }
        }
    }

    LOG_I("✓ calib_coeff.json loaded (%d parameters)",
          calibrationData.parameterCount);
    return true;
}

// =========================
// HELPER FUNCTIONS
// =========================

CalibrationCoeffs *findCalibrationById(const String &id)
{
    for (uint8_t i = 0; i < calibrationData.parameterCount; i++)
    {
        if (calibrationData.parameters[i].id == id)
        {
            return &calibrationData.parameters[i];
        }
    }
    return nullptr;
}

void printConfiguration()
{
    LOG_I("📋 Current configuration:");
    LOG_I("   WiFi: %s", systemConfig.mainMcu.wifiSsid.c_str());
    LOG_I("   Sample Interval: %dms", systemConfig.interfaceMcu.sampleIntervalMs);
    LOG_I("   Sensors configured: %d", systemConfig.interfaceMcu.parameterCount);

    for (uint8_t i = 0; i < systemConfig.interfaceMcu.parameterCount; i++)
    {
        LOG_I("     ID=%s: %s [%s]",
              systemConfig.interfaceMcu.parameters[i].id.c_str(),
              systemConfig.interfaceMcu.parameters[i].name.c_str(),
              systemConfig.interfaceMcu.parameters[i].unit.c_str());
    }
}

void printCalibration()
{
    LOG_I("Calibration coefficients:");

    for (uint8_t i = 0; i < calibrationData.parameterCount; i++)
    {
        LOG_I("   ID=%s (%s): %d coefficients",
               calibrationData.parameters[i].id.c_str(),
               calibrationData.parameters[i].name.c_str(),
               calibrationData.parameters[i].coeffCount);

        for (uint8_t j = 0; j < calibrationData.parameters[i].coeffCount; j++)
        {
            LOG_I("     Coeff[%d] = %.6f",
                  j + 1, calibrationData.parameters[i].coeffs[j]);
        }
    }
}
