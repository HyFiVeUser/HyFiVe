/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Global system variables and configurations
 */

#ifndef SYSTEMVARIABLES_H
#define SYSTEMVARIABLES_H

#include <Arduino.h>
#include <atomic>

#include "MQTTManager.h"
#include "loggerConfig.h"

extern std::atomic<bool> ledMeasurementsOff;

// Configuration variables

inline float fwVersionLoggerMainboard = 0.01;
inline int maxMeasurementCountForLed = 5;           // in count
inline int sampleCastIntervals = 3;                 // in count
inline int waitAfterUnderwaterMeasurementTime = 30; // in seconds

// Variables for the periods

inline uint32_t config_update_periode = configRTC.config_update_periode;         // configRTC.config_update_periode;
inline uint32_t status_upload_periode = configRTC.status_upload_periode;         // configRTC.status_upload_periode;
inline uint32_t wet_det_periode = configRTC.wet_det_periode;                     // configRTC.wet_det_periode;
inline uint32_t data_upload_retry_periode = configRTC.data_upload_retry_periode; // configRTC.data_upload_retry_periode;

// General variables

inline uint32_t minTimeUntilNextFunction = 0;
inline bool hasWifiConnection = false;
inline bool isNtpSynchronized = false;
inline bool hasSensorError = false;
inline RTC_DATA_ATTR LoggerConfigRTC configRTC;
inline RTC_DATA_ATTR int WifiArraySize = 0;
inline RTC_DATA_ATTR int lastSuccessfulNetworkIndex = -1;
inline RTC_DATA_ATTR int SensorArraySize = 0;

// Sensor-specific variables

inline RTC_DATA_ATTR uint8_t needVoltage = 0;
inline RTC_DATA_ATTR uint8_t temperatureSensorBusAddress = 255;
inline RTC_DATA_ATTR uint8_t temperatureSensorParameter = 255;
inline RTC_DATA_ATTR uint8_t oxygenSensorBusAddress = 255;
inline RTC_DATA_ATTR uint8_t waterDetectionSensorBusAddress = 0;
inline RTC_DATA_ATTR uint8_t dryDetectionSensorBusAddress = 0;
inline RTC_DATA_ATTR uint8_t castDetectionSensorBusAddress = 0;
inline RTC_DATA_ATTR uint8_t dryDetSensorParameterNo = 0;
inline RTC_DATA_ATTR uint8_t castDetSensorParameterNo = 0;
inline RTC_DATA_ATTR uint8_t wetDetSensorParameterNo = 0;
inline RTC_DATA_ATTR uint32_t saveSamplePeriodeToResetAfterUnderwaterMeasurementsEnd = 0;
inline RTC_DATA_ATTR uint8_t errorSkipSensor[32] = {};
inline RTC_DATA_ATTR uint8_t errorSkipSensorSize = 0;

// Time-related variables

inline RTC_DATA_ATTR uint32_t totalElapsedTime = 0;
inline RTC_DATA_ATTR uint32_t lastConfigUpdateTime = 0;       // configRTC.config_update_periode;
inline RTC_DATA_ATTR uint32_t lastStatusUploadTime = 0;       // configRTC.status_upload_periode;
inline RTC_DATA_ATTR uint32_t lastWetDetectionUploadTime = 0; // configRTC.wet_det_periode;
inline RTC_DATA_ATTR uint32_t lastDataUploadRetryTime = 0;    // data_upload_retry_periode
inline RTC_DATA_ATTR time_t currentTimeNow = 0;
inline RTC_DATA_ATTR time_t waitAfterUnderwaterMeasurementTimeNow = 0;
inline uint8_t mqttErrorCounter = 0;
inline RTC_DATA_ATTR uint8_t currentIncorrectNumberOfsensors = 0;
inline RTC_DATA_ATTR uint32_t secInactivityTimeoutSec = 0;
inline RTC_DATA_ATTR uint32_t inactivityTimeoutSec = 900; // seconds

// Status information

inline RTC_DATA_ATTR bool isFirstBoot = false;
inline RTC_DATA_ATTR bool isLoggerSubmerged = false;
inline RTC_DATA_ATTR bool hasStatusUploadError = false;
inline RTC_DATA_ATTR bool hasTransmissionUpdateError = false;
inline RTC_DATA_ATTR bool hasMqttHeaderError = false;
inline RTC_DATA_ATTR bool hasMqttLogError = false;
inline RTC_DATA_ATTR bool hasMqttMeasurementError = false;
inline RTC_DATA_ATTR bool isDataUploadRetryEnabled = false;
inline bool isNodeRedAvailable = false;
inline bool getNodeRedBusyCheck = false;
inline bool isNodeRedBusy = true;
inline bool isNodeRedLogin = false;
inline bool isNodeRedStatus = false;
inline bool isFirmwareUpdate = false;
inline bool isLoggerBusy = false;
inline RTC_DATA_ATTR bool chargingStatus = false;
inline RTC_DATA_ATTR bool batteryCompletlyCharged = false;
inline RTC_DATA_ATTR bool wificonfigRtc = false;
inline RTC_DATA_ATTR bool batteryEmpty = false;
inline RTC_DATA_ATTR bool bmsConfig = false;
inline bool interfaceError = false;
inline RTC_DATA_ATTR bool statusDeepSleep = true;
inline RTC_DATA_ATTR std::atomic<bool> statusReedInput{true};

// Counter variables

inline RTC_DATA_ATTR int bootAttemptCount = 0;
inline RTC_DATA_ATTR int detectionThresholdValue = 0;
inline RTC_DATA_ATTR int numberOfActiveSensors = 0;
inline RTC_DATA_ATTR int boot = 0;
inline RTC_DATA_ATTR int configurationBootCount = 0;
inline RTC_DATA_ATTR int totalMeasurementCount = 0;
inline RTC_DATA_ATTR int bootCounter = 0;
inline RTC_DATA_ATTR uint16_t longestSensorWakeupTime = 0;
inline RTC_DATA_ATTR uint8_t interfaceRdyErrorCounter = 0;
inline RTC_DATA_ATTR uint8_t sensorCalibToInterfaceIfRdyErrorCounter = 0;

// Measurement data variables

inline RTC_DATA_ATTR float totalOperationTime = 0;
inline RTC_DATA_ATTR float lastSensorMeasurementTime[MAX_SENSOR_CREDENTIALS] = {};
inline RTC_DATA_ATTR float intervalSensorArray[MAX_SENSOR_CREDENTIALS];
inline RTC_DATA_ATTR bool thresholdValuewaterDetection = false;
inline RTC_DATA_ATTR uint32_t deployment_id = 0;
inline RTC_DATA_ATTR uint32_t interfaceErrorSensorId = 0;

// File processing variables

inline RTC_DATA_ATTR long totalMeasurementLines = 0;
inline RTC_DATA_ATTR bool countTotalMeasurementLines = true;

// LED
inline RTC_DATA_ATTR bool ledSensorTrue = false;
inline RTC_DATA_ATTR bool setBeginDeployment = true;
inline RTC_DATA_ATTR bool sensorStartDone = false;
inline RTC_DATA_ATTR bool statusLED = false;
inline RTC_DATA_ATTR uint32_t lowDurationMs = 0;
inline RTC_DATA_ATTR uint32_t lowStartMs = 0;
inline RTC_DATA_ATTR uint16_t ledColorConfig = 231; // RGB color selection (hundreds: normal, tens: interaction, ones: errors)
inline RTC_DATA_ATTR uint8_t rgbVersion = 0;        // RGB hardware version selector

#endif
