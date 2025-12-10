/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Logger configuration management
 */

#ifndef LOGGERCONFIG_H
#define LOGGERCONFIG_H

// Maximum number of SENSOR
#define MAX_SENSOR_CREDENTIALS 32

// Maximum number of WIFI SENSORs
#define MAX_WIFI_CREDENTIALS 5

typedef struct
{
  float calib_coeff_1;
  float calib_coeff_2;
  float calib_coeff_3;
  float calib_coeff_4;
  float calib_coeff_5;
  float calib_coeff_6;
  float calib_coeff_7;
  float calib_coeff_8;
  float calib_coeff_9;
  float calib_coeff_10;
  uint8_t sensor_type_id;
  char long_name[101];
  char unit[46];
  char manufacturer[46];
  char model[46];
  char serial_number[46];
  float accuracy;
  float resolution;
} Sensor;

typedef struct
{
  char operation_mode[46];
  char fw_version[101];
  uint16_t deckunit_id;
  uint16_t platform_id;
  uint16_t vessel_id;
  char vessel_name[46];
  uint16_t deployment_contact_id;
  char contact_first_name[46];
  char contact_last_name[46];
  Sensor sensor[MAX_SENSOR_CREDENTIALS];
} LoggerConfig;

typedef struct
{
  uint16_t sensor_id;
  uint8_t sample_periode_multiplier;
  uint8_t sample_cast_periode_multiplier;
  uint8_t bus_address;
  char parameter[46];
  uint8_t parameter_no;
} SensorRTC;

typedef struct
{
  char ssid[16];
  char pw[17];
} WifiConfigRTC;

typedef struct
{
  uint16_t logger_id;
  uint8_t num_sensors;
  uint32_t config_update_periode;
  uint32_t status_upload_periode;
  uint16_t sample_periode;
  bool sample_cast_enable;
  uint8_t sample_cast_periode;
  uint16_t cast_det_sensor;
  float cast_det_sensor_threshold;
  uint16_t wet_det_sensor;
  uint16_t wet_det_periode;
  float wet_det_threshold;
  uint16_t dry_det_sensor;
  float dry_det_threshold;
  uint16_t dry_det_verify_delay;
  uint16_t data_upload_retry_periode;
  SensorRTC sensor[MAX_SENSOR_CREDENTIALS];
  WifiConfigRTC wificonfig[MAX_WIFI_CREDENTIALS];
} LoggerConfigRTC;

extern LoggerConfigRTC configRTC;
extern LoggerConfig config;

bool compareRtcWithJsonConfig();
bool JsonFileRead(const String &pfad);

void configureSensorsFromJson();
void configureWifiFromJson();
void configureBasicSettingsFromJson();
void validateAndLoadConfig();

#endif