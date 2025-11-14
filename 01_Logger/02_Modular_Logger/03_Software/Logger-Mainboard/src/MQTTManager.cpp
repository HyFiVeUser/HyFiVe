/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: MQTT communication management
 */

#include <ArduinoJson.h>
#include <MQTT.h>
#include <WiFi.h>
#include <regex>

#include "BMS.h"
#include "DS3231TimeNtp.h"
#include "DebuggingSDLog.h"
#include "Led.h"
#include "MQTTManager.h"
#include "SensorManagement.h"
#include "SystemVariables.h"
#include "Utility.h"
#include "WifiNetwork.h"
#include "firmwareUpdate.h"
#include "loggerConfig.h"

#define MMMS 1024 // MAX_MQTT_MESSAGE_SIZE

WiFiClient wifi;
MQTTClient client(512, 512); // Read/write buffer size = 512

const char *mqttHost = "192.168.1.1";
const int mqttPort = 1883;
const char *mqttName = "HyFiVe";
const char *mqttUser = "";
const char *mqttPassword = "";

/**
 * @brief Transmits an update message via MQTT.
 * @param updateInfo The update message to send.
 * @param mqtt_topic The MQTT topic to publish to.
 * @return true if the message was sent successfully, false otherwise.
 */
bool transmitUpdateMessage(const char *updateInfo, const char *mqtt_topic)
{
  uint8_t errorCount = 0;
  while (1)
  {
    connectToMqtt(); // Establishes the connection to MQTT, if not already done.
    if (hasWifiConnection)
    {
      if (client.connected())
      {
        if (client.publish(mqtt_topic, updateInfo, false, 2) == 0)
        {
          hasTransmissionUpdateError = true;
          Log(LogCategoryMQTT, LogLevelDEBUG, "The message could not be sent");
        }
        else
        {
          hasTransmissionUpdateError = false;
          Log(LogCategoryMQTT, LogLevelDEBUG, "The message was sent successfully");
          return true;
        }
      }
      else
      {
        hasTransmissionUpdateError = true;
        Log(LogCategoryMQTT, LogLevelDEBUG, "MQTT client is not connected");
      }
    }
    else
    {
      return false;
    }

    if (errorCount >= 10)
    {
      Log(LogCategoryMQTT, LogLevelDEBUG, "transmitUpdateMessage: ", updateInfo, " ", mqtt_topic);
      Log(LogCategoryMQTT, LogLevelDEBUG, "The message could not be sent");
      return false;
    }
    errorCount++;
  }
}

/**
 * @brief Uploads the logger status via MQTT.
 */
void uploadStatus()
{
  char mqtt_topic[] = "hyfive/status";

  StaticJsonDocument<200> doc;

  doc["logger_id"] = configRTC.logger_id;
  doc["battery_remaining"] = getRemainingBatteryPercentage();
  doc["memory_capacity_total"] = sdCardSpaceTotal();
  doc["memory_capacity_used"] = sdCardSpaceUsed();

  char payload[MMMS];

  serializeJson(doc, payload, sizeof(payload));

  connectToMqtt();
  if (hasWifiConnection)
  {
    if (client.connected())
    {
      if (client.publish(mqtt_topic, payload, false, 2) == 0)
      {
        Log(LogCategoryMQTT, LogLevelDEBUG, "statusUpload could not be transmitted");
        hasStatusUploadError = true;
        return;
      }
      else
      {
        hasStatusUploadError = false;
        Log(LogCategoryMQTT, LogLevelDEBUG, "statusUpload successfully transferred");
        return;
      }
    }
    else
    {
      Log(LogCategoryMQTT, LogLevelDEBUG, "statusUpload could not be transmitted");
      hasStatusUploadError = true;
      return;
    }
  }
  else
  {
    Log(LogCategoryMQTT, LogLevelDEBUG, "statusUpload could not be transmitted no wifi");
    return;
  }
}

/**
 * @brief Moves a file with a timestamp.
 * @param sourceFolder The source folder of the file.
 * @param fileName The name of the file.
 * @param destinationFolder The destination folder for the file.
 * @return true if the file was successfully moved, otherwise false.
 */
bool moveFileWithTimestamp(const char *sourceFolder, const char *fileName, const char *destinationFolder)
{
  String sourcePath = String(sourceFolder) + "/" + fileName;
  String destinationPath = String(destinationFolder) + "/" + fileName;

  if (!SD.rename(sourcePath.c_str(), destinationPath.c_str()))
  {
    // Attempt to delete the file
    if (SD.remove(sourcePath.c_str()))
    {
      Log(LogCategoryMQTT, LogLevelERROR, "File deleted after failed move: ", fileName);
      return false;
    }
    else
    {
      Log(LogCategoryMQTT, LogLevelERROR, "File could not be moved or deleted: ", fileName);
      return false;
    }
  }
  return true;
}

/**
 * @brief Extracts a timestamp from a file name.
 * @param filename The filename from which the timestamp is to be extracted.
 * @return The extracted timestamp.
 * @throws std::runtime_error if no valid timestamp was found in the file name.
 */
std::string extractTimestamp(const std::string &dateiname)
{
  std::regex muster("logger_\\d+_config_(\\d{12})\\.json");
  std::smatch treffer;

  if (std::regex_search(dateiname, treffer, muster) && treffer.size() > 1)
  {
    return treffer[1].str(); // The extracted timestamp
  }
  else
  {
    throw std::runtime_error("No valid timestamp found in the file name.");
  }
}

/**
 * @brief Compares two timestamps and determines whether the first is older than the second.
 * @param file1 The first file name with timestamp.
 * @param file2 The second file name with timestamp.
 * @return true if the timestamp in file1 is older than that in file2, otherwise false.
 */
bool isFirstFileOlder(const std::string &datei1, const std::string &datei2)
{
  std::string zeitstempel1 = extractTimestamp(datei1);
  std::string zeitstempel2 = extractTimestamp(datei2);

  // Compare the timestamps as strings
  return zeitstempel1 < zeitstempel2;
}

/**
 * @brief Checks whether a message corresponds to the expected format.
 * @param nachricht The message to be checked.
 * @return true if the message corresponds to the expected format, otherwise false.
 */
bool checkMessageFormat(const std::string &nachricht)
{
  std::regex muster("logger_\\d+_config_\\d{12}\\.json");
  // Checks whether the message matches the pattern.
  return std::regex_match(nachricht, muster);
}

/**
 * @brief Checks a message and extracts logger ID and timestamp.
 * @param message The message to be checked.
 * @param loggerID The extracted logger ID.
 * @param timestamp The extracted timestamp.
 * @return true if logger ID and timestamp were successfully extracted, otherwise false.
 */
bool checkAndExtractNumbers(const std::string &nachricht, std::string &loggerID, std::string &timestamp)
{
  std::regex muster("^logger_(\\d+)_config_(\\d{12})\\.json$");
  std::smatch treffer;

  if (std::regex_match(nachricht, treffer, muster))
  {
    // Extract the numbers from the groups
    loggerID = treffer[1].str();
    timestamp = treffer[2].str();
    return true;
  }
  return false;
}

bool timestampError = false;
bool loggerIDError = false;

bool noUpdateAvaiable = true;
bool configUpdateAvaiable = true;
bool FWUpdateAvaiable = true;
bool messageMqttReceive = false;
unsigned long lastMessageTime = 0;
bool readFileIn = false;
File dataFile;
String received_sha256 = "";

/**
 * @brief Received MQTT message.
 * @param client MQTT client.
 * @param topic MQTT topic of the received message.
 * @param payload The received message.
 * @param length The length of the received message.
 */
void handleReceivedMessage(MQTTClient *client, char *topic, char *payload, int length)
{
  String messageTemp;
  if (noUpdateAvaiable)
  {
    for (int i = 0; i < length; i++)
    {
      messageTemp += (char)payload[i];
    }

    messageTemp.trim();

    if ((messageTemp == String(configRTC.logger_id) + "_0") && isNodeRedStatus)
    {
      isNodeRedBusy = false;
      return;
    }

    if ((messageTemp == String(configRTC.logger_id)) && isNodeRedLogin)
    {
      isNodeRedAvailable = true;
      return;
    }

    if (messageTemp == "no_firmware_update")
    {
      messageMqttReceive = true;
      isFirmwareUpdate = true;
      Log(LogCategoryMQTT, LogLevelDEBUG, "no_firmware_update");
      return;
    }

    if (messageTemp == "no_update_available")
    {
      messageMqttReceive = true;
      Log(LogCategoryMQTT, LogLevelDEBUG, "no config update available");
      return;
    }
    noUpdateAvaiable = false;
  }

  if (FWUpdateAvaiable)
  {
    if (String(topic) == "hyfive/updateFirmwareSHA256")
    {
      messageMqttReceive = true;
      noUpdateAvaiable = false;

      received_sha256 = messageTemp;

      if (SD.exists("/updateFW/current_firmware.sha256"))
      {
        File file = SD.open("/updateFW/current_firmware.sha256");

        if (file)
        {
          String stored_sha256 = file.readStringUntil('\n');
          stored_sha256.trim();
          file.close();

          if (stored_sha256 == received_sha256)
          {
            Serial.println("sha256 value is equal");
            isFirmwareUpdate = true;
            return;
          }
        }
      }

      FWUpdateAvaiable = false;
      readFileIn = true;

      dataFile = SD.open("/updateFW/firmware.bin", FILE_APPEND);
      if (!dataFile)
      {
        Serial.println("Error opening the file on the SD card.");
      }
      else
      {
        transmitUpdateMessage((messageTemp).c_str(), "hyfive/updateFWRequest");
        return;
      }
    }
  }

  if (configUpdateAvaiable)
  {
    std::string loggerID;
    std::string timestamp;

    // Check the message.
    if (checkAndExtractNumbers(messageTemp.c_str(), loggerID, timestamp))
    {
      messageMqttReceive = true;

      if (configRTC.logger_id == std::stoi(loggerID))
      {
        loggerIDError = false;
        Log(LogCategoryGeneral, LogLevelDEBUG, "Logger ID Matches in: ", String(messageTemp));
      }
      else
      {
        Log(LogCategoryGeneral, LogLevelERROR, "Error in: ", String(messageTemp), " - ", "Logger ID incorrect");
        transmitUpdateMessage(("Error in: " + messageTemp + " - " + "Logger ID incorrect").c_str(), "hyfive/ConfigError");
        loggerIDError = true;
        return;
      }

      timestampError = false;
      if (isFirstFileOlder(std::string(findLatestConfigurationFile("/loggerConfig").c_str()), std::string(messageTemp.c_str())))
      {
        configUpdateAvaiable = false;
        readFileIn = true;

        deleteAllFilesInFolder("/updateConfig");

        // Construction of the complete path
        String pfad = "/updateConfig/" + messageTemp;

        dataFile = SD.open(pfad.c_str(), FILE_APPEND);
        if (!dataFile)
        {
          Serial.println("Error opening the file on the SD card.");
        }
        else
        {
          // File opened on SD card.
          transmitUpdateMessage((messageTemp).c_str(), "hyfive/updateConfigUpload");
          return;
        }
      }
      else
      {
        transmitUpdateMessage(("Info: " + messageTemp + " - " + "latest update on the logger is: " + findLatestConfigurationFile("/loggerConfig")).c_str(), "hyfive/ConfigError");
        return;
      }
    }
    else
    {
      transmitUpdateMessage(("Error in: " + messageTemp + " - " + "File name not compliant").c_str(), "hyfive/ConfigError");
    }
  }

  if (readFileIn)
  {
    for (int i = 0; i < length; i++)
    {
      // Serial.print(payload[i]);
      dataFile.print(payload[i]);
    }
  }

  lastMessageTime = millis();
}

/**
 * Checks whether there is an error in the logger ID or timestamp.
 * @return bool True if either loggerIDError or timestampError is true, otherwise false.
 */
bool errorInloggerIdOrTimestamp()
{
  if (loggerIDError || timestampError)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * @brief Establishes a connection to the MQTT broker.
 * @return true if the connection was successfully established, otherwise false.
 */
bool connectToMqtt()
{
  connectToWifiAndSyncNTP();
  if (hasWifiConnection)
  {
    if (!client.connected())
    {
      Log(LogCategoryMQTT, LogLevelDEBUG, "Trying to connect to the MQTT server...");

      client.begin(mqttHost, mqttPort, wifi);

      if (client.connect(mqttName, mqttUser, mqttPassword))
      {
        Log(LogCategoryMQTT, LogLevelDEBUG, "Connected to MQTT broker.");

        // Subscribe to the topics
        client.subscribe("hyfive/updateConfig", 2);
        client.subscribe("hyfive/updateFirmwareSHA256", 2);
        client.subscribe("hyfive/updateFW", 0);
        client.subscribe("hyfive/nodeRedLogin", 2);
        client.subscribe("hyfive/nodeRedBusyStatus", 2);

        // Set up callback function for incoming messages
        client.onMessageAdvanced(handleReceivedMessage);

        return true;
      }
      else
      {
        Log(LogCategoryMQTT, LogLevelDEBUG, "Error: Could not connect to MQTT broker.");
        return false;
      }
    }

    return true; // Already connected with MQTT
  }
  else
  {
    Log(LogCategoryMQTT, LogLevelDEBUG, "Error: No WiFi connection. Cannot connect to MQTT broker.");
    return false;
  }
}

/**
 * @brief Updates the logger configuration via MQTT.
 * @note Timeout or completion will end the process
 */
void updateConfigViaMqtt()
{
  lastMessageTime = millis();
  while (1)
  {
    client.loop();
    delay(10);

    if (millis() - lastMessageTime > 1000)
    {
      if (!messageMqttReceive)
      {
        Log(LogCategoryGeneral, LogLevelDEBUG, "no message received from the deckbox");
        transmitUpdateMessage(("Error in: updateConfig - no message received from the deckbox"), "hyfive/ConfigError");
      }

      if (readFileIn)
      {
        dataFile.close();
      }
      readFileIn = false;
      break;
    }
  }
}

/**
 * @brief Performs firmware update via MQTT.
 *        Receives firmware data through MQTT messages and verifies SHA256.
 * @note Timeout or completion will end the process
 */
void updateFWViaMqtt()
{
  isFirmwareUpdate = false;
  configUpdateAvaiable = false;
  lastMessageTime = millis();
  while (1)
  {
    client.loop();
    delay(10);

    if (millis() - lastMessageTime > 1500)
    {

      if (readFileIn)
      {
        dataFile.close();
      }
      readFileIn = false;

      noUpdateAvaiable = true;
      FWUpdateAvaiable = true;
      if (!isFirmwareUpdate)
      {
        if (calculateSha256(received_sha256))
        {
          Serial.println("SHA256 Hash matches!");
          isFirmwareUpdate = true;
          client.unsubscribe("hyfive/updateFW");
          client.unsubscribe("hyfive/updateFirmwareSHA256");
        }
        else
        {
          Serial.println("SHA256 Hash mismatch!");

          if (!SD.exists("/updateFW/firmware.bin"))
          {
            isFirmwareUpdate = true;
          }
          else
          {
            SD.remove("/updateFW/firmware.bin");
            isFirmwareUpdate = false;
          }
        }
      }
      break;
    }
  }
}

/**
 * @brief Requests the status of Node-RED.
 */
void requestNodeRedStatus()
{
  isNodeRedLogin = true;
  isNodeRedAvailable = false;
  transmitUpdateMessage((String(configRTC.logger_id)).c_str(), "hyfive/nodeRedRequest");
  lastMessageTime = millis();
  while (1)
  {
    client.loop();
    delay(10);

    if (isNodeRedAvailable)
    {
      Log(LogCategoryMQTT, LogLevelDEBUG, "Node-RED is available / requestNodeRedStatus");
      break;
    }

    if (millis() - lastMessageTime > 500)
    {
      client.unsubscribe("hyfive/nodeRedLogin");
      isNodeRedLogin = false;
      break;
    }
  }
}

/**
 * @brief Requests the busy status of Node-RED.
 */
void requestNodeRedBusyStatus()
{
  isNodeRedStatus = true;
  isNodeRedBusy = true;
  transmitUpdateMessage((String(configRTC.logger_id)).c_str(), "hyfive/nodeRedBusyRequest");
  lastMessageTime = millis();
  while (1)
  {
    client.loop();
    delay(10);

    if (!isNodeRedBusy)
    {
      Log(LogCategoryMQTT, LogLevelDEBUG, "Node-RED is not busy");
      client.unsubscribe("hyfive/nodeRedBusyStatus");
      isNodeRedStatus = false;
      break;
    }

    if (millis() - lastMessageTime > 500)
    {
      Log(LogCategoryMQTT, LogLevelINFO, "Node-RED is busy");
      client.unsubscribe("hyfive/nodeRedBusyStatus");
      isNodeRedStatus = false;
      break;
    }
  }
}

/**
 * @brief Checks if Node-RED is responsive.
 * @return true if Node-RED is responsive, false otherwise.
 */
bool isNodeRedResponsePositive()
{
  if (isNodeRedAvailable)
  {
    Log(LogCategoryMQTT, LogLevelDEBUG, "Node-RED is available");
    return true;
  }
  else
  {
      Log(LogCategoryMQTT, LogLevelERROR, "Node-RED is not available");
    return false;
  }
}

/**
 * @brief Moves measurement data to appropriate directories.
 */
void moveMeasurementAndData()
{
  // Check if there are measurement JSON files in the "/measurements" directory
  if (checkFileProperties("/measurements", 0, ".json"))
  {
    // Move the config header file to the MQTT header directory
    moveFileToDestination("/measurements", "configHeader.json", "/measurements/mqtt_header", true);

    // Move the measurement file to the MQTT measurements directory
    moveFileToDestination("/measurements", "measurement.json", "/measurements/mqtt_measurements", true);
  }
}

/**
 * @brief Processes and transmits measurement data.
 */
void processAndTransmitMeasurementData()
{
  moveMeasurementAndData();

  // Check if there are any files in the MQTT header or measurements or log directories
  if (checkFileProperties("/measurements/mqtt_header", 0, ".json") || checkFileProperties("/measurements/mqtt_measurements", 0, ".json") || checkFileProperties("/log", 0, ".txt"))
  {
    while (checkWetSensorAndNodeRed())
    {
      // Transmit all header data while header files are present
      if (checkFileProperties("/measurements/mqtt_header", 0, ".json"))
      {
        if (connectToMqtt())
        {
          transmitHeaderViaMqtt();
        }
      }

      // Transmit all measurement data while measurement files are present
      if (checkFileProperties("/measurements/mqtt_measurements", 0, ".json"))
      {
        if (connectToMqtt())
        {
          transmitDataViaMqtt();
        }
      }

      // Transmit all log data while files are present
      if (checkFileProperties("/log", 0, ".txt"))
      {
        if (connectToMqtt())
        {
          transmitLogViaMqtt();
        }
      }

      // Check if there are not measurement JSON files in the "/measurements" directory
      if (!checkFileProperties("/measurements/mqtt_header", 0, ".json") && !checkFileProperties("/measurements/mqtt_measurements", 0, ".json"))
      {
        Log(LogCategoryMQTT, LogLevelDEBUG, "Data successfully transmitted");
        loggerTransmittedMeasurementDataLED();
        break;
      }
      if (mqttErrorCounter == 20)
      {
        Log(LogCategoryMQTT, LogLevelERROR, "bad wifi connection");
        break;
      }
    }
  }
}

struct TransmissionState
{
  char filename[55];
  long lineNumber;
  long totalLines;
};

RTC_DATA_ATTR TransmissionState rtcHeaderState;
RTC_DATA_ATTR TransmissionState rtcDataState;
RTC_DATA_ATTR TransmissionState rtcLogState;

/**
 * @brief Saves the current transmission status for header data.
 *
 * This function updates the saved status of the header data transmission,
 * to be able to continue the transmission in the event of interruptions or restarts.
 *
 * @param state The saved transmission state.
 */
void saveHeaderTransmissionState(const TransmissionState &state)
{
  rtcHeaderState = state;
}

/**
 * @brief Saves the current transmission status for log data.
 *
 * This function updates the saved status of the header data transmission,
 * to be able to continue the transmission in the event of interruptions or restarts.
 *
 * @param state The saved transmission state.
 */
void saveLogTransmissionState(const TransmissionState &state)
{
  rtcLogState = state;
}

/**
 * @brief Loads the saved transmission status for header data.
 * @return bool True if a state has been loaded, otherwise False.
 */
bool loadHeaderTransmissionState(TransmissionState &state)
{
  if (rtcHeaderState.filename[0] != '\0')
  {
    state = rtcHeaderState;
    return true;
  }
  return false;
}

/**
 * @brief Loads the saved transmission status for log data.
 * @return bool True if a state has been loaded, otherwise False.
 */
bool loadLogTransmissionState(TransmissionState &state)
{
  if (rtcLogState.filename[0] != '\0')
  {
    state = rtcLogState;
    return true;
  }
  return false;
}

/**
 * @brief Saves the current transmission status for measurement data.
 *
 * This function updates the saved status of the measurement data transmission,
 * to be able to continue the transmission in the event of interruptions or restarts.
 *
 * @param state The saved transmission state.
 */
void saveDataTransmissionState(const TransmissionState &state)
{
  rtcDataState = state;
}

/**
 * @brief Loads the saved transmission status for measurement data.
 * @return bool True if a state has been loaded, otherwise False.
 */
bool loadDataTransmissionState(TransmissionState &state)
{
  if (rtcDataState.filename[0] != '\0')
  {
    state = rtcDataState;
    return true;
  }
  return false;
}

/**
 * @brief Transmits header data via MQTT.
 * @return true if the transmission was successful, otherwise false.
 */
bool transmitHeaderViaMqtt()
{
  char mqtt_topic[] = "hyfive/header";
  bool pass = true;

  File dir = SD.open("/measurements/mqtt_header");
  if (!dir)
  {
    Serial.println("Directory could not be opened");
    return false;
  }
  if (!dir.isDirectory())
  {
    Serial.println("No directory");
    dir.close();
    return false;
  }

  File currentHeaderFile;
  TransmissionState headerState;
  if (loadHeaderTransmissionState(headerState))
  {
    // Saved transmission status for header found
    String basePath = "/measurements/mqtt_header/";
    currentHeaderFile = SD.open(basePath + headerState.filename);
    if (currentHeaderFile)
    {
      // Jump to the saved line number
      for (long i = 0; i < headerState.lineNumber; i++)
      {
        currentHeaderFile.readStringUntil('\n');
      }
    }
  }
  else
  {
    currentHeaderFile = dir.openNextFile();
    if (currentHeaderFile)
    {
      strncpy(headerState.filename, currentHeaderFile.name(), sizeof(headerState.filename));
      headerState.lineNumber = 0;

      // Count the total number of lines in the file
      headerState.totalLines = 0;
      while (currentHeaderFile.available())
      {
        currentHeaderFile.readStringUntil('\n');
        headerState.totalLines++;
      }

      // Set the file pointer back to the beginning
      currentHeaderFile.seek(0);
    }
  }

  while (currentHeaderFile)
  {

    // if (currentHeaderFile.available() || (headerState.lineNumber != headerState.totalLines))
    if (currentHeaderFile.available() && headerState.lineNumber < headerState.totalLines)
    {
      char payload[MMMS];
      String buf = currentHeaderFile.readStringUntil('\n');

      // Skip empty lines
      if (buf.length() == 0)
      {
        headerState.lineNumber++;
        continue;
      }

      buf.toCharArray(payload, sizeof(payload));

      if (client.publish(mqtt_topic, payload, false, 1) == 0)

      {
        Log(LogCategoryMQTT, LogLevelDEBUG, "MQTT Disconnection: ", "filename: ", String(headerState.filename), " | ", String(headerState.lineNumber), "/", String(headerState.totalLines));
        pass = false;
        saveHeaderTransmissionState(headerState);
        hasMqttHeaderError = true;
        mqttErrorCounter++;
        break;
      }
      else
      {
        headerState.lineNumber++;
        saveHeaderTransmissionState(headerState);
        hasMqttHeaderError = false;
      }
    }
    else
    {
      currentHeaderFile.close();
      if (headerState.lineNumber == headerState.totalLines)
      {
        String basePath = "/measurements/mqtt_header";
        moveFileWithTimestamp("/measurements/mqtt_header", headerState.filename, "/backup/header");
        Log(LogCategoryMQTT, LogLevelINFO, "header lines transmitted: ", "filename: ", String(headerState.filename), " | ", String(headerState.lineNumber), "/", String(headerState.totalLines));
        memset(headerState.filename, '\0', sizeof(headerState.filename));
        headerState.lineNumber = 0;
        headerState.totalLines = 0;
        saveHeaderTransmissionState(headerState);
      }
    }
  }

  dir.close();
  return pass;
}

/**
 * @brief Transmits Log data via MQTT.
 * @return true if the transmission was successful, otherwise false.
 */
bool transmitLogViaMqtt()
{
  char mqtt_topic[] = "hyfive/Log";
  bool pass = true;

  File dir = SD.open("/log");
  if (!dir)
  {
    Serial.println("Directory could not be opened");
    return false;
  }
  if (!dir.isDirectory())
  {
    Serial.println("No directory");
    dir.close();
    return false;
  }

  File currentLogFile;
  TransmissionState LogState;
  if (loadLogTransmissionState(LogState))
  {
    // Saved transmission status for log found
    String basePath = "/log/";
    currentLogFile = SD.open(basePath + LogState.filename);
    if (currentLogFile)
    {
      // Jump to the saved line number
      for (long i = 0; i < LogState.lineNumber; i++)
      {
        currentLogFile.readStringUntil('\n');
      }
    }
  }
  else
  {
    currentLogFile = dir.openNextFile();
    if (currentLogFile)
    {
      strncpy(LogState.filename, currentLogFile.name(), sizeof(LogState.filename));
      LogState.lineNumber = 0;

      // Count the total number of lines in the file
      LogState.totalLines = 0;
      while (currentLogFile.available())
      {
        currentLogFile.readStringUntil('\n');
        LogState.totalLines++;
      }

      // Set the file pointer back to the beginning
      currentLogFile.seek(0);
    }
  }

  while (currentLogFile)
  {

    // if (currentLogFile.available() || (LogState.lineNumber != LogState.totalLines))
    if (currentLogFile.available() && LogState.lineNumber < LogState.totalLines)
    {
      char payload[MMMS];
      String buf = currentLogFile.readStringUntil('\n');

      // Skip empty lines
      if (buf.length() == 0)
      {
        LogState.lineNumber++;
        continue;
      }

      buf.toCharArray(payload, sizeof(payload));

      if (client.publish(mqtt_topic, payload, false, 1) == 0)

      {
        Log(LogCategoryMQTT, LogLevelDEBUG, "MQTT Disconnection: ", "filename: ", String(LogState.filename), " | ", String(LogState.lineNumber), "/", String(LogState.totalLines));
        pass = false;
        saveLogTransmissionState(LogState);
        hasMqttLogError = true;
        mqttErrorCounter++;
        break;
      }
      else
      {
        LogState.lineNumber++;
        saveLogTransmissionState(LogState);
        hasMqttLogError = false;
      }
    }
    else
    {
      currentLogFile.close();
      if (LogState.lineNumber == LogState.totalLines)
      {
        moveLogToBackup();
        memset(LogState.filename, '\0', sizeof(LogState.filename));
        LogState.lineNumber = 0;
        LogState.totalLines = 0;
        saveLogTransmissionState(LogState);
      }
    }
  }

  dir.close();
  return pass;
}

/**
 * @brief Transmits measurement data via MQTT.
 * @return true if the transmission was successful, otherwise false.
 */
bool transmitDataViaMqtt()
{
  char mqtt_topic[] = "hyfive/data";
  bool pass = true;

  File dir = SD.open("/measurements/mqtt_measurements");
  if (!dir)
  {
    Serial.println("Directory could not be opened");
    return false;
  }
  if (!dir.isDirectory())
  {
    Serial.println("No directory");
    dir.close();
    return false;
  }

  File currentMeasurementFile;
  TransmissionState dataState;
  if (loadDataTransmissionState(dataState))
  {
    String basePath = "/measurements/mqtt_measurements/";
    currentMeasurementFile = SD.open(basePath + dataState.filename);
    if (currentMeasurementFile)
    {
      // Jump to the saved line number
      for (long i = 0; i < dataState.lineNumber; i++)
      {
        currentMeasurementFile.readStringUntil('\n');
      }
    }
  }
  else
  {
    currentMeasurementFile = dir.openNextFile();
    if (currentMeasurementFile)
    {
      strncpy(dataState.filename, currentMeasurementFile.name(), sizeof(dataState.filename));
      dataState.lineNumber = 0;

      // Count the total number of lines in the file
      dataState.totalLines = 0;
      while (currentMeasurementFile.available())
      {
        currentMeasurementFile.readStringUntil('\n');
        dataState.totalLines++;
      }

      // Set the file pointer back to the beginning
      currentMeasurementFile.seek(0);
    }
  }

  while (currentMeasurementFile)
  {

    // if (currentMeasurementFile.available() || (dataState.lineNumber != dataState.totalLines))
    if (currentMeasurementFile.available() && dataState.lineNumber < dataState.totalLines)
    {
      char payload[MMMS];
      String buf = currentMeasurementFile.readStringUntil('\n');

      // Skip empty lines
      if (buf.length() == 0)
      {
        dataState.lineNumber++;
        continue;
      }

      buf.toCharArray(payload, sizeof(payload));

      if (client.publish(mqtt_topic, payload, false, 1) == 0)
      {
        Log(LogCategoryMQTT, LogLevelDEBUG, "MQTT Disconnection: ", "filename: ", String(dataState.filename), " | ", String(dataState.lineNumber), "/", String(dataState.totalLines));
        pass = false;
        saveDataTransmissionState(dataState);
        hasMqttMeasurementError = true;
        mqttErrorCounter++;
        break;
      }
      else
      {
        dataState.lineNumber++;
        saveDataTransmissionState(dataState);
        hasMqttMeasurementError = false;
      }
    }
    else
    {
      currentMeasurementFile.close();
      if (dataState.lineNumber == dataState.totalLines)
      {
        String basePath = "/measurements/mqtt_measurements";
        moveFileWithTimestamp("/measurements/mqtt_measurements", dataState.filename, "/backup/measurements");
        Log(LogCategoryMQTT, LogLevelINFO, "data lines transmitted: ", "filename: ", String(dataState.filename), " | ", String(dataState.lineNumber), "/", String(dataState.totalLines));
        memset(dataState.filename, '\0', sizeof(dataState.filename));
        dataState.lineNumber = 0;
        dataState.totalLines = 0;
        saveDataTransmissionState(dataState);
      }
    }
  }

  dir.close();
  return pass;
}