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

#ifndef UTILITY_H
#define UTILITY_H

// System initialization

void performFirstBootOperations();
void createRequiredFolders();
bool checkWetSensorAndNodeRed();
void handleSensorError(uint16_t threshold);
bool isPowerSupplyConnected();

// Configuration

void performPeriodicConfigUpdate();
String findLatestConfigurationFile(const String &pfad);
bool checkLatestConfigFileExists();
extern String findLatestConfigFileUpdateConfig;
extern bool sReset;
void deleteAllFilesInFolder(String path);
bool moveFileToDestination(const char *sourceFolder, const char *fileName, const char *destinationFolder, bool addTimestamp);
void appendDataToFile(const String &filename, const String &data);
void writeDeploymentIdToFile();
bool checkFileProperties(const String &path, size_t minSize = 0, const String &extension = "", const String &inhalt = "");
int64_t calculateAvailableSdCardSpace();
int64_t sdCardSpaceTotal();
int64_t sdCardSpaceUsed();
void checkWetSensorThreshold();
void moveLogToBackup();

// Energy management

void connectionOfPowerSupplyBeginChargingOfBatteries();
void batteryRemainingLow(uint8_t batteryCharge);
void enable3V3();
void enable12V();
void enable5V();
void disable3V3();
void disable12V();
void disable5V();
void interfaceRST();

// Time control

uint32_t calculateShortestWaitTime(uint32_t totalElapsedTime,
                                   uint32_t lastConfigUpdateTime,
                                   uint32_t lastStatusUploadTime,
                                   uint32_t lastWetDetectionUploadTime,
                                   uint32_t lastDataUploadRetryTime,
                                   bool isDataUploadRetryEnabled,
                                   uint32_t config_update_periode,
                                   uint32_t status_upload_periode,
                                   uint32_t wet_det_periode,
                                   uint32_t data_upload_retry_periode);
void resetTimePeriodeLoop(uint32_t config_update_periode, uint32_t status_upload_periode, uint32_t wet_det_periode, uint32_t data_upload_retry_periode);
void configUpdatePeriodeFunktion(uint32_t config_update_periode);
void statusUploadPeriodeFunktion(uint32_t status_upload_periode);
void wetDetPeriodeFunktion(uint32_t wet_det_periode);
void dataUploadRetryPeriodeFunktion(uint32_t data_upload_retry_periode);
void getFirmwareUpdate();

void askForConfigReq();
void reedMonitorInit();
void holdOnReedInputPressed();

#endif