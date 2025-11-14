/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Utility functions for various system operations
 */

#include <ArduinoJson.h>
#include <SD.h>
#include <vector>

#include "BMS.h"
#include "Charger.h"
#include "DS3231TimeNtp.h"
#include "DebuggingSDLog.h"
#include "DeepSleep.h"
#include "LED.h"
#include "MQTTManager.h"
#include "SDCard.h"
#include "SensorManagement.h"
#include "SystemVariables.h"
#include "Utility.h"
#include "WifiNetwork.h"
#include "firmwareUpdate.h"
#include "loggerConfigValidation.h"

int loggerIdUpdate = 0;
String findLatestConfigFileUpdateConfig;

/**
 * @brief Copies a file from a source folder to a destination folder.
 * @param sourceFolder The source folder of the file.
 * @param fileName The name of the file to be copied.
 * @param destinationFolder The destination folder for the file.
 * @return true if the file was copied successfully, otherwise false.
 */
bool copyFileToDestination(const char *sourceFolder, const char *fileName, const char *destinationFolder)
{
  String sourcePath = String(sourceFolder) + "/" + String(fileName);
  String destinationPath = String(destinationFolder) + "/" + String(fileName);

  File sourceFile = SD.open(sourcePath.c_str(), FILE_READ);
  if (!sourceFile)
  {
    Serial.println("Cannot open source file");
    return false;
  }

  File destinationFile = SD.open(destinationPath.c_str(), FILE_WRITE);
  if (!destinationFile)
  {
    Serial.println("Cannot open target file");
    sourceFile.close();
    return false;
  }

  // Copy file
  size_t size = sourceFile.size();
  size_t done = 0;
  byte buf[512];
  while (done < size)
  {
    size_t len = min(sizeof(buf), size - done);
    sourceFile.read(buf, len);
    destinationFile.write(buf, len);
    done += len;
  }

  // Close files
  sourceFile.close();
  destinationFile.close();

  delay(100);
  return true;
}

/**
 * @brief Moves a file to a destination folder, optionally adding a timestamp.
 * @param sourceFolder Source folder path.
 * @param fileName Name of the file to move.
 * @param destinationFolder Destination folder path.
 * @param addTimestamp Whether to add a timestamp to the filename.
 * @return true if the file was successfully moved, false otherwise.
 */
bool moveFileToDestination(const char *sourceFolder, const char *fileName, const char *destinationFolder, bool addTimestamp = false)
{
  if (addTimestamp)
  {
    String timestamp = getLocalTimeAsStringBackup();

    if (timestamp == "")
    {
      return false;
    }
    String sourcePath = String(sourceFolder) + "/" + fileName;
    String newFileName = String(destinationFolder) + "/" + "logger_" + configRTC.logger_id + "_" + timestamp + "_" + fileName;

    if (!SD.rename(sourcePath.c_str(), newFileName.c_str()))
    {
      return false;
    }

    delay(100);
    return true;
  }
  else
  {
    String sourcePath = String(sourceFolder) + "/" + fileName;
    String destinationPath = String(destinationFolder) + "/" + fileName;

    if (SD.exists(destinationPath.c_str()))
    {
      // file already exists, try deleting it before moving it
      if (!SD.remove(destinationPath.c_str()))
      {
        Serial.println("Destination file could not be deleted: " + destinationPath);
        return false;
      }
    }

    if (!SD.rename(sourcePath.c_str(), destinationPath.c_str()))
    {
      Serial.println("File could not be moved: " + sourcePath);
      return false;
    }

    delay(100);
    return true;
  }
}

/**
 * @brief Writes deployment ID to a file.
 */
void writeDeploymentIdToFile()
{
  if (!SD.exists("/measurements/deployment_id.txt"))
  {
    File file = SD.open("/measurements/deployment_id.txt", FILE_WRITE);
    if (!file)
    {
      Serial.println("Error while creating the file!");
      return;
    }
    file.print("deployment_id = 1;");
    file.close();
    deployment_id = 1;
  }
  else
  {
    File file = SD.open("/measurements/deployment_id.txt", FILE_READ);
    if (!file)
    {
      Serial.println("Error opening the file!");
      return;
    }

    if (file.available())
    {
      String line = file.readStringUntil('\n');
      int index = line.indexOf("deployment_id = ");
      if (index != -1)
      {
        String number = line.substring(index + strlen("deployment_id = "));
        deployment_id = number.toInt();
      }
    }
    file.close();

    deployment_id++;

    file = SD.open("/measurements/deployment_id.txt", FILE_WRITE);
    if (!file)
    {
      Serial.println("Error when opening the file for writing!");
      return;
    }

    file.print("deployment_id = ");
    file.print(deployment_id);
    file.println(";");

    file.close();
  }
  delay(100);
}

/**
 * @brief Appends data to a specified file.
 * @param filename Name of the file to append to.
 * @param data Data to append.
 */
void appendDataToFile(const String &filename, const String &data)
{
  File datei = SD.open(filename, FILE_APPEND);
  if (datei)
  {
    datei.println(data);
    datei.close();
  }
  else
  {
    Serial.println("Error opening the file");
  }
}

/**
 * @brief Checks various properties of a file or folder.
 * @param path Path to the file or folder.
 * @param minSize Minimum size of the file (0 for no size check).
 * @param extension File extension to check for (empty for no extension check).
 * @param inhalt Content to write if creating a new file (empty for no file creation).
 * @return true if all checks pass, false otherwise.
 */
bool checkFileProperties(const String &path, size_t minSize, const String &extension, const String &inhalt)
{
  File file = SD.open(path);
  if (!file)
  {
    // Attempts to create the file if 'content' is not empty
    if (inhalt.length() > 0)
    {
      file = SD.open(path, FILE_WRITE);
      if (file)
      {
        file.println(inhalt);
        file.close();
        return true; // File successfully created
      }
      return false; // File could not be created
    }
    return false; // File or directory does not exist and no content specified for creation
  }

  if (file.isDirectory())
  {
    // Check for files with a specific extension in a directory
    if (extension.length() > 0)
    {
      File child = file.openNextFile();
      while (child)
      {
        if (String(child.name()).endsWith(extension))
        {
          child.close();
          file.close();
          return true; // file with the specified extension found
        }
        child.close();
        child = file.openNextFile();
      }
      file.close();
      return false; // No file with the specified extension found
    }
  }
  else
  {
    // Check the file size if minSize is specified
    if (minSize > 0 && file.size() < minSize)
    {
      file.close();
      return false; // File is smaller than minSize
    }
  }

  file.close();
  return true; // File exists and fulfills the size condition or no specific condition specified
}

/**
 * @brief Deletes all files in a specified folder.
 * @param path Path to the folder.
 */
void deleteAllFilesInFolder(String path)
{
  File dir = SD.open(path);
  if (!dir)
  {
    Serial.println("Failed to open folder: " + path);
    return;
  }
  if (!dir.isDirectory())
  {
    Serial.println("This is not a folder: " + path);
    dir.close();
    return;
  }

  File file = dir.openNextFile();
  while (file)
  {
    if (!file.isDirectory())
    {
      String filePath = path + "/" + file.name();
      SD.remove(filePath.c_str());
      Serial.println("Deleted: " + filePath);
    }
    file = dir.openNextFile();
  }

  dir.close();
}

/**
 * @brief Creates required folders on the SD card.
 */
void createRequiredFolders()
{
  const char *folders[] = {
      "/backup",
      "/backup/config",
      "/backup/config_error",
      "/backup/log_error",
      "/backup/log",
      "/backup/header",
      "/backup/measurements",
      "/loggerConfig",
      "/log",
      "/measurements",
      "/measurements/mqtt_header",
      "/measurements/mqtt_measurements",
      "/updateConfig",
      "/updateFW"};
  const int numFolders = sizeof(folders) / sizeof(folders[0]);

  for (int i = 0; i < numFolders; ++i)
  {
    if (SD.exists(folders[i]))
    {
      Log(LogCategoryGeneral, LogLevelDEBUG, "Folder ", folders[i], " already exists.");
    }
    else
    {
      if (SD.mkdir(folders[i]))
      {
        Log(LogCategoryGeneral, LogLevelDEBUG, "Folder ", folders[i], " created.");
      }
      else
      {
        Log(LogCategoryGeneral, LogLevelERROR, "Error when creating the folder ", folders[i]);
        generalAlarmLed();
      }
    }
  }
}

/**
 * @brief Enables 3.3V power supply.
 */
void enable3V3()
{
  pinMode(GPIO_NUM_10, OUTPUT);
  gpio_hold_dis(GPIO_NUM_10);
  digitalWrite(GPIO_NUM_10, HIGH);
  delay(500);
  gpio_hold_en(GPIO_NUM_10);
  delay(100);
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
 * @brief Disables 3.3V power supply.
 */
void disable3V3()
{
  pinMode(GPIO_NUM_10, OUTPUT);
  gpio_hold_dis(GPIO_NUM_10);
  digitalWrite(GPIO_NUM_10, LOW);
  delay(500);
  gpio_hold_en(GPIO_NUM_10);
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

/**
 * @brief Resets the interface.
 */
void interfaceRST()
{
  pinMode(GPIO_NUM_7, OUTPUT);
  gpio_hold_dis(GPIO_NUM_7);
  digitalWrite(GPIO_NUM_7, LOW);
  delay(10);
  gpio_hold_en(GPIO_NUM_7);
  delay(1000);
  pinMode(GPIO_NUM_7, OUTPUT);
  gpio_hold_dis(GPIO_NUM_7);
  digitalWrite(GPIO_NUM_7, HIGH);
  delay(500);
  gpio_hold_en(GPIO_NUM_7);
  delay(3000);
}

/**
 * @brief Performs first boot operations.
 */
void performFirstBootOperations()
{
  if (!isFirstBoot)
  {
    Log(LogCategoryGeneral, LogLevelINFO, "-------------------Start System initialization-------------------");
    Log(LogCategoryGeneral, LogLevelINFO, "Logger-Mainboard: FWVersion: ", String(fwVersionLoggerMainboard));
    firstBootLed();
    createRequiredFolders();
    interfaceSleep();
    updateFirmware();
    validateAndLoadConfig();
    connectToWifiAndSyncNTP();
    manageBatteryCharging();
    firstBootLed();
    interfaceRST();
    sensorAvailability();
    statusUploadPeriodeFunktion(0);
    configUpdatePeriodeFunktion(0);
    isfirstBootLed = false;
    handleNtpSynchronization();
    currentTimeNow = getCurrentTimeFromRTC();
    isFirstBoot = true;
    Log(LogCategoryGeneral, LogLevelINFO, "---------------------End System initialization-------------------");
    espDeepSleepSec(0);
  }
}

/**
 * @brief Moves the log file from the source directory to the backup directory.

 * This function performs the following operations:
 * 1. Checks if the source file exists.
 * 2. Creates the backup directory if it doesn't exist.
 * 3. If the backup file already exists, appends the content of the source file to it.
 * 4. If the backup file doesn't exist, renames the source file to the backup file.
 * 5. Deletes the original source file after successful backup.
 */
void moveLogToBackup()
{

  const char *sourceFile = "/log/log.txt";
  const char *backupDir = "/backup/log";
  const char *backupFile = "/backup/log/log.txt";

  // Check if source file exists
  if (!SD.exists(sourceFile))
  {
    Serial.println("Source file does not exist!");
    return;
  }

  // Create backup directory if it doesn't exist
  if (!SD.exists(backupDir))
  {
    SD.mkdir(backupDir);
  }

  File source = SD.open(sourceFile, FILE_READ);
  if (!source)
  {
    Serial.println("Error opening source file!");
    return;
  }

  // If backup file already exists, append to it
  if (SD.exists(backupFile))
  {
    File destination = SD.open(backupFile, FILE_APPEND);
    if (!destination)
    {
      Serial.println("Error opening backup file for appending!");
      source.close();
      return;
    }

    while (source.available())
    {
      destination.write(source.read());
    }

    destination.println(); // Add a newline for separation
    destination.close();
  }
  // If backup file doesn't exist, just rename the source file
  else
  {
    source.close();
    if (SD.rename(sourceFile, backupFile))
    {
      Serial.println("File moved successfully!");
    }
    else
    {
      Serial.println("Error moving file!");
      return;
    }
  }

  source.close();

  // Delete the original file if it still exists
  if (SD.exists(sourceFile))
  {
    SD.remove(sourceFile);
  }

  Serial.println("Log backup completed successfully!");
}

bool findLatestConfigFileLog = true;

/**
 * @brief Checks if the latest configuration file exists.
 * @return true if the file exists, false otherwise.
 */
bool checkLatestConfigFileExists()
{
  if (findLatestConfigFileLog)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * @brief Finds the latest configuration file.
 * @param pfad Path to search for configuration files.
 * @return String Name of the latest configuration file.
 */
String findLatestConfigurationFile(const String &pfad)
{
  findLatestConfigFileLog = true;

  File root = SD.open(pfad);

  // Creates a vector that will store the names of the configuration update files
  std::vector<String> configUpdateFileNames;

  // Iterates over all files in the /config directory
  while (true)
  {
    File file = root.openNextFile(); // Opens the next file in the directory
    if (!file)                       // If no more files are available, the loop ends
    {
      break;
    }

    if (!file.isDirectory()) // Checks whether the opened file is not a directory
    {
      String configUpdateFileName = file.name(); // Saves the name of the file
      // Filtert nur JSON-Dateien
      if (configUpdateFileName.endsWith(".json")) // Checks whether the file extension is .json
      {
        configUpdateFileNames.push_back(configUpdateFileName); // Adds the file name to the vector
      }
    }
    file.close(); // Closes the currently open file
  }

  // Sort the files by timestamp
  std::sort(configUpdateFileNames.begin(), configUpdateFileNames.end(), [](const String &a, const String &b) -> bool
            {
              // Extract timestamp directly before the .json extension
              int posA = a.lastIndexOf('_') + 1;
              int posB = b.lastIndexOf('_') + 1;
              String timestampA = a.substring(posA, a.length() - 5); // -5 to remove “.json”
              String timestampB = b.substring(posB, b.length() - 5); // -5 to remove “.json”
              return timestampA > timestampB;                        // Descending sorting for the most recent timestamp first
            });

  // Checks whether the list of file names is not empty
  if (!configUpdateFileNames.empty())
  {
    // Saves the name of the most recent configuration file
    String latestFileName = configUpdateFileNames.front();
    return latestFileName; // The newest file is now the first in the vector
  }
  else
  {
    findLatestConfigFileLog = false;
  }
  return "";
}

/**
 * @brief Performs periodic configuration update.
 */
void performPeriodicConfigUpdate()
{
  deleteAllFilesInFolder("/updateConfig");

  Log(LogCategoryConfiguration, LogLevelDEBUG, "current ConfigFile: ", String(findLatestConfigurationFile("/loggerConfig").c_str()));
  transmitUpdateMessage(findLatestConfigurationFile("/loggerConfig").c_str(), "hyfive/updateConfigRequest");

  if (!hasTransmissionUpdateError)
  {
    updateConfigViaMqtt();
  }

  if (!hasTransmissionUpdateError)
  {
    findLatestConfigFileUpdateConfig = findLatestConfigurationFile("/updateConfig");

    if (checkLatestConfigFileExists() && !errorInloggerIdOrTimestamp())
    {
      if (JsonFileReadValidation("/updateConfig/" + findLatestConfigFileUpdateConfig))
      {
        validateBasicConfiguration();
        validateWifiConfiguration();
        validateSensorsConfiguration();

        if (configUpdateError)
        {
          moveFileToDestination("/updateConfig", (findLatestConfigurationFile("/updateConfig")).c_str(), "/backup/config_error");
          return;
        }
      }
      else
      {
        transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "JSON deserialization failed").c_str(), "hyfive/ConfigError");
        moveFileToDestination("/updateConfig", (findLatestConfigurationFile("/updateConfig")).c_str(), "/backup/config_error");
        return;
      }

      if (configRTC.logger_id == logger_idValidation)
      {
        Log(LogCategoryConfiguration, LogLevelINFO, "update successfull: ", String(findLatestConfigFileUpdateConfig));
        transmitUpdateMessage(("update successfull: " + findLatestConfigFileUpdateConfig).c_str(), "hyfive/updateConfigRequest");
        if (!hasTransmissionUpdateError)
        {
          deleteAllFilesInFolder("/loggerConfig");
          moveFileToDestination("/updateConfig", (findLatestConfigurationFile("/updateConfig")).c_str(), "/loggerConfig");
          copyFileToDestination("/loggerConfig/", (findLatestConfigurationFile("/loggerConfig")).c_str(), "/backup/config");
          ESP.restart();
        }
      }
      else
      {
        transmitUpdateMessage(("Error in: " + findLatestConfigFileUpdateConfig + " - " + "logger_id now: " + configRTC.logger_id + " | " + "logger_id update: " + logger_idValidation).c_str(), "hyfive/ConfigError");
        if (!hasTransmissionUpdateError)
        {
          moveFileToDestination("/updateConfig", (findLatestConfigurationFile("/updateConfig")).c_str(), "/backup/config_error");
        }
        return;
      }
    }
  }
}

/**
 * @brief Manages firmware update process.
 *        Checks conditions, downloads firmware via MQTT, and initiates update.
 * @note Attempts download up to 3 times before failing
 */
void getFirmwareUpdate()
{
  if (checkWetSensorAndNodeRed())
  {
    if (SD.exists("/updateFW/firmware.bin"))
    {
      SD.remove("/updateFW/firmware.bin");
    }
    for (int i = 0; i < 3 && !isFirmwareUpdate; i++)
    {
      transmitUpdateMessage("fwRequest", "hyfive/updateFwSHA256Request");
      updateFWViaMqtt();
    }
    if (!isFirmwareUpdate)
    {
      Log(LogCategoryGeneral, LogLevelERROR, "firmware download not successful");
    }
    //! transmitUpdateMessage("fw_download_successfull", "hyfive/updateFwSHA256Request");
    updateFirmware();
  }
}

/**
 * @brief Calculates available space on the SD card.
 * @return int64_t Available space in bytes.
 */
int64_t calculateAvailableSdCardSpace()
{
  // SD card size
  int64_t cardSize = SD.cardSize() / (1024 * 1024);

  // Memory space used
  int64_t usedSpace = (SD.usedBytes() / (1024 * 1024));

  // Free storage space
  int64_t freeSpace = cardSize - usedSpace; // Free memory in megabytes
  return freeSpace;
}

int64_t sdCardSpaceTotal()
{
  return SD.cardSize();
}

int64_t sdCardSpaceUsed()
{
  return SD.usedBytes();
}

/**
 * @brief Calculates the shortest wait time until the next function execution.
 * @param totalElapsedTime Total elapsed time.
 * @param lastConfigUpdateTime Last configuration update time.
 * @param lastStatusUploadTime Last status upload time.
 * @param lastWetDetectionUploadTime Last wet detection upload time.
 * @param lastDataUploadRetryTime Last data upload retry time.
 * @param isDataUploadRetryEnabled Flag indicating if data upload retry is enabled.
 * @param config_update_periode Configuration update period.
 * @param status_upload_periode Status upload period.
 * @param wet_det_periode Wet detection period.
 * @param data_upload_retry_periode Data upload retry period.
 * @return uint32_t The shortest wait time in seconds.
 */
uint32_t calculateShortestWaitTime(uint32_t totalElapsedTime, uint32_t lastConfigUpdateTime, uint32_t lastStatusUploadTime, uint32_t lastWetDetectionUploadTime, uint32_t lastDataUploadRetryTime, bool isDataUploadRetryEnabled, uint32_t config_update_periode, uint32_t status_upload_periode, uint32_t wet_det_periode, uint32_t data_upload_retry_periode)
{
  uint32_t shortestWaitTimePeriode = ULONG_MAX;
  uint32_t intervalPeriodeArray[4] = {config_update_periode, status_upload_periode, wet_det_periode, ULONG_MAX};
  uint32_t lastMeasurePeriode[4] = {lastConfigUpdateTime, lastStatusUploadTime, lastWetDetectionUploadTime, lastDataUploadRetryTime};
  int numberOfActiveSensorsCalc = 3;

  if (isDataUploadRetryEnabled)
  {
    intervalPeriodeArray[3] = data_upload_retry_periode;
    numberOfActiveSensorsCalc = 4;
  }

  for (int i = 0; i < numberOfActiveSensorsCalc; i++)
  {
    uint32_t timeUntilNextMeasurement = intervalPeriodeArray[i] - (totalElapsedTime - lastMeasurePeriode[i]);
    if (timeUntilNextMeasurement < shortestWaitTimePeriode)
    {
      shortestWaitTimePeriode = timeUntilNextMeasurement;
    }
  }
  return shortestWaitTimePeriode;
}

bool askForConfigRequest = false;

/**
 * @brief Checks for configuration update request.
 *
 * @example
 * askForConfig();
 */
void askForConfig()
{
  if (!isLoggerSubmerged)
  {
    if (digitalRead(17) == LOW)
    {
      pinMode(42, OUTPUT);
      gpio_hold_dis(GPIO_NUM_42);

      digitalWrite(42, HIGH);
      delay(500);
    }
    else
    {
      setResetEspLow();
      return;
    }

    if (digitalRead(17) == LOW)
    {
      Log(LogCategoryConfiguration, LogLevelDEBUG, "askForConfigRequest");
      askForConfigRequest = true;
    }
    else
    {
      return;
    }

    if (digitalRead(17) == LOW)
    {
      digitalWrite(42, LOW);
      delay(500);
    }
    else
    {
      setResetEspLow();
      return;
    }

    if (digitalRead(17) == LOW)
    {
      digitalWrite(42, HIGH);
      delay(500);
    }
    else
    {
      setResetEspLow();
      return;
    }

    if (digitalRead(17) == LOW)
    {
      digitalWrite(42, LOW);
      delay(500);
    }
    else
    {
      setResetEspLow();
      return;
    }

    if (digitalRead(17) == LOW)
    {
      digitalWrite(42, HIGH);
      delay(500);
    }
    else
    {
      setResetEspLow();
      return;
    }

    if (digitalRead(17) == LOW)
    {
      digitalWrite(42, LOW);
      delay(500);
    }
    else
    {
      setResetEspLow();
      return;
    }

    while (digitalRead(17) == LOW)
    {
      digitalWrite(42, HIGH);
    }

    setResetEspLow();
  }
}

/**
 * @brief Handles the connection of power supply and starts charging batteries.
 */
void connectionOfPowerSupplyBeginChargingOfBatteries()
{
  pinMode(20, INPUT);
  int pin20Status = digitalRead(20);
  if (pin20Status == LOW && (getRemainingBatteryPercentage() < 100) && !batteryCompletlyCharged)
  {
    uint8_t wakeUpTime = 0;

    if (!chargingStatus)
    {
      batteryEmpty = false;
      chargingStatus = true;
      // enable3V3();
    }

    connectionOfPowerSupplyBeginChargingOfBatteriesLED();

    if (minTimeUntilNextFunction > wakeUpTime)
    {
      minTimeUntilNextFunction = wakeUpTime;
    }
  }
  if (pin20Status == HIGH)
  {
    batteryCompletlyCharged = false;
    chargingStatus = false;
    // disable3V();
  }
}

/**
 * @brief Handles low battery scenarios.
 * @param batteryCharge The current battery charge percentage.
 */
void batteryRemainingLow(uint8_t batteryCharge)
{
  pinMode(20, INPUT);
  int pin20Status_ = digitalRead(20);
  if (pin20Status_ == HIGH) // Power supply unit not connected when HIGH
  {
    if (getRemainingBatteryPercentage() == 0 || getTotalBatteryCellVoltage() < 11200)
    {
      if (!batteryEmpty)
      {
        Log(LogCategoryPowerManagement, LogLevelDEBUG, "batteryRemaining: ", String(getRemainingBatteryPercentage()), " %");
        Log(LogCategoryPowerManagement, LogLevelINFO, "Battery empty");
        enableExternalWakeup(20); // if Power supply connected = LOW
        enableExternalWakeup(17); // when reed switch is actuated
        batteryEmpty = true;
        esp_deep_sleep_start();
      }
    }

    if (getRemainingBatteryPercentage() <= batteryCharge)
    {
      Log(LogCategoryPowerManagement, LogLevelDEBUG, "batteryRemaining: ", String(getRemainingBatteryPercentage()), " %");
      enableExternalWakeup(20);
      enableExternalWakeup(17);
      batteryRemainingLowLED();
    }
  }
}

/**
 * @brief Programs the BMS (Battery Management System).
 *
 * Can only be programmed if battery is above 11.2V and power supply is connected charging does not have to be.
 */
void programBms()
{
  disable3V3();
  Log(LogCategoryBMS, LogLevelINFO, "bmsProg");
  enableExternalWakeup(17); // reed switch
  esp_deep_sleep_start();
}

/**
 * @brief Resets time period loop when total elapsed time exceeds the largest period.
 *
 * @param config_update_periode Configuration update period.
 * @param status_upload_periode Status upload period.
 * @param wet_det_periode Wet detection period.
 * @param data_upload_retry_periode Data upload retry period.
 */
void resetTimePeriodeLoop(uint32_t config_update_periode, uint32_t status_upload_periode, uint32_t wet_det_periode, uint32_t data_upload_retry_periode)
{
  uint32_t largestNumber = max({config_update_periode, status_upload_periode, wet_det_periode, data_upload_retry_periode});

  if (totalElapsedTime >= largestNumber)
  {
    totalElapsedTime = 0;
    lastConfigUpdateTime = 0;
    lastStatusUploadTime = 0;
    lastWetDetectionUploadTime = 0;
    lastDataUploadRetryTime = 0;
  }
}

/**
 * @brief Checks if power supply is connected.
 * @return true if power supply is connected, false otherwise.
 */
bool isPowerSupplyConnected()
{
  pinMode(20, INPUT);
  int pin20Status = digitalRead(20);
  if (pin20Status == LOW) // if Power supply connected = LOW
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * @brief Handles scenarios when the battery is completely charged.
 */
void batteryCompletelyCharged()
{
  pinMode(9, INPUT);  // STAT1
  pinMode(19, INPUT); // STAT2
  pinMode(20, INPUT); // PG

  int pin9Status = digitalRead(9);
  int pin19Status = digitalRead(19);
  int pin20Status = digitalRead(20);

  Log(LogCategoryCharger, LogLevelINFO, "STAT1: ", String(pin9Status));
  Log(LogCategoryCharger, LogLevelINFO, "STAT2: ", String(pin19Status));
  Log(LogCategoryCharger, LogLevelINFO, "PG: ", String(pin20Status));

  if (pin20Status == LOW) // if power supply connected = LOW
  {
    uint8_t wakeUpTime = 0;
    disableWakeupPin(20);

    if (minTimeUntilNextFunction > wakeUpTime)
    {
      minTimeUntilNextFunction = wakeUpTime;
    }

    if (batteryCompletlyCharged)
    {
      batteryCompletelyChargedLED();
    }

    Log(LogCategoryGeneral, LogLevelDEBUG, "batteryRemaining: ", String(getRemainingBatteryPercentage()), " %");

    if (getRemainingBatteryPercentage() >= 100 && !batteryCompletlyCharged)
    {
      // disable3V();
      Log(LogCategoryPowerManagement, LogLevelINFO, "Battery completly charged");
      batteryCompletlyCharged = true;
    }

    if (!batteryCompletlyCharged && getRemainingBatteryPercentage() >= 91 && getCellCurrent())
    {
      Log(LogCategoryPowerManagement, LogLevelDEBUG, "Battery completly charged > 100%");
      Log(LogCategoryGeneral, LogLevelDEBUG, "batteryRemaining: ", String(getRemainingBatteryPercentage()), " %");
      bmsReset();
    }
  }
  else
  {
    enableExternalWakeup(20); // Activate power supply connection
  }
  enableExternalWakeup(17); // Activate reed connection
}

/**
 * @brief Handles configuration update period functionality.
 * @param config_update_periode Configuration update period.
 */
void configUpdatePeriodeFunktion(uint32_t config_update_periode)
{
  if (((totalElapsedTime - lastConfigUpdateTime) >= config_update_periode) || hasTransmissionUpdateError || askForConfigRequest)
  {
    uint8_t errorCount = 0;
    while (1)
    {
      Log(LogCategoryGeneral, LogLevelDEBUG, "config_update_periode");
      if (checkWetSensorAndNodeRed())
      {
        performPeriodicConfigUpdate();
        getFirmwareUpdate();

        isDataUploadRetryEnabled = hasTransmissionUpdateError;
      }

      if (errorCount >= 10)
      {
        Log(LogCategoryMQTT, LogLevelERROR, "configUpdate could not be transmitted");
        break;
      }

      if (!hasTransmissionUpdateError)
      {
        break;
      }
      errorCount++;
    }
    lastConfigUpdateTime = totalElapsedTime;
  }
}

/**
 * @brief Handles status upload period functionality.
 * @param status_upload_periode Status upload period.
 */
void statusUploadPeriodeFunktion(uint32_t status_upload_periode)
{
  if (((totalElapsedTime - lastStatusUploadTime) >= status_upload_periode) || hasStatusUploadError == true)
  {
    uint8_t errorCount = 0;
    while (1)
    {
      Log(LogCategoryGeneral, LogLevelDEBUG, "status_upload_periode");
      if (checkWetSensorAndNodeRed())
      {
        uploadStatus();
        isDataUploadRetryEnabled = hasStatusUploadError;
      }

      if (errorCount >= 10)
      {
        Log(LogCategoryMQTT, LogLevelERROR, "statusUpload could not be transmitted");
        break;
      }

      if (!hasStatusUploadError)
      {
        break;
      }
      errorCount++;
    }
    lastStatusUploadTime = totalElapsedTime;
  }
}

/**
 * @brief Handles wet detection period functionality.
 * @param wet_det_periode Wet detection period.
 */
void wetDetPeriodeFunktion(uint32_t wet_det_periode)
{
  if ((totalElapsedTime - lastWetDetectionUploadTime) >= wet_det_periode)
  {
    bootCounter++;
    // batteryRemainingLow(15); // Battery charge below 15% or 0%
    checkWetSensorThreshold();
    lastWetDetectionUploadTime = totalElapsedTime;
  }
}

/**
 * @brief Handles data upload retry period functionality.
 * @param data_upload_retry_periode Data upload retry period.
 */
void dataUploadRetryPeriodeFunktion(uint32_t data_upload_retry_periode)
{
  if ((isDataUploadRetryEnabled && (totalElapsedTime - lastDataUploadRetryTime) >= data_upload_retry_periode) || hasMqttHeaderError || hasMqttMeasurementError)
  {
    if (hasTransmissionUpdateError)
    {
      Log(LogCategoryGeneral, LogLevelERROR, "data_upload_retry_periode || timeConfigUpdatePeriode");
    }

    if (hasStatusUploadError)
    {
      Log(LogCategoryGeneral, LogLevelERROR, "data_upload_retry_periode || statusUpload");
    }

    if (hasMqttHeaderError)
    {
      Log(LogCategoryGeneral, LogLevelERROR, "data_upload_retry_periode || hasMqttHeaderError");
      isDataUploadRetryEnabled = true;
    }

    if (hasMqttMeasurementError)
    {
      Log(LogCategoryGeneral, LogLevelERROR, "data_upload_retry_periode || hasMqttMeasurementError");
      isDataUploadRetryEnabled = true;
    }

    lastDataUploadRetryTime = totalElapsedTime;
  }
  else
  {
    isDataUploadRetryEnabled = false;
  }
}

/**
 * @brief Checks the connection to the humidity sensor and the availability of Node-RED.
 *
 * This function performs two important checks:
 * 1. it checks whether the humidity sensor detects water.
 * 2. it checks the availability of Node-RED.
 *
 * @return bool Returns true if the sensor does not detect moisture and Node-RED is available.
 * Returns false if either moisture is detected or Node-RED is not available.
 */
bool checkWetSensorAndNodeRed()
{
  if (checkWetSensorStatus())
  {
    return false;
  }

  if (connectToWifiAndSyncNTP())
  {
    uint8_t errorCount = 0;

    if (!getNodeRedBusyCheck)
    {
      requestNodeRedBusyStatus();
      getNodeRedBusyCheck = true;
    }

    if (isNodeRedBusy)
    {
      return false;
    }

    while (1)
    {
      requestNodeRedStatus();
      if (isNodeRedAvailable)
      {
        break;
      }
      if (errorCount >= 10)
      {
        break;
      }
      errorCount++;
    }

    if (isNodeRedResponsePositive())
    {
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}

/**
 * @brief Handles sensor errors.
 */
void handleSensorError(uint16_t threshold)
{
  if (!chargingStatus)
  {
    if (bootCounter >= threshold)
    {
      compareRtcWithJsonConfig();  //* RTC configuration comparison
      performInitialMeasurement(); //* Sensor error detection
      bootCounter = 0;
    }
  }
}
