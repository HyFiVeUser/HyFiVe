/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: WiFi network connection and management
 */

#include <WiFi.h>

#include "DebuggingSDLog.h"
#include "Led.h"
#include "SystemVariables.h"

/**
 * @brief Connects to Wi-Fi and synchronizes time with NTP.
 * @return true if connection and synchronization were successful, false otherwise.
 */
bool connectToWifiAndSyncNTP()
{
  if (waitAfterUnderwaterMeasurementTimeNow >= getCurrentTimeFromRTC())
  {
    Log(LogCategoryWiFi, LogLevelDEBUG, "waitAfterUnderwaterMeasurement");
    return false;
  }

  if (strlen(configRTC.wificonfig[0].ssid) == 0 || strlen(configRTC.wificonfig[0].pw) == 0)
  {
    Log(LogCategoryWiFi, LogLevelWARNING, "No WLAN configuration found in the RTC memory");
    hasWifiConnection = false;
    wificonfigRtc = false;
    return false;
  }
  else
  {
    wificonfigRtc = true;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Log(LogCategoryWiFi, LogLevelDEBUG, "WLAN is still connected.");
    hasWifiConnection = true;
    return true;
  }
  else
  {
    // First try the last successful network if we have one
    if (lastSuccessfulNetworkIndex >= 0 && lastSuccessfulNetworkIndex < WifiArraySize)
    {
      if (strlen(configRTC.wificonfig[lastSuccessfulNetworkIndex].ssid) > 0)
      {
        Log(LogCategoryWiFi, LogLevelDEBUG, "Trying to reconnect to last successful network: ", 
            String(configRTC.wificonfig[lastSuccessfulNetworkIndex].ssid));
        
        for (int attempt = 0; attempt < 2; ++attempt)
        {
          WiFi.begin(configRTC.wificonfig[lastSuccessfulNetworkIndex].ssid, 
                    configRTC.wificonfig[lastSuccessfulNetworkIndex].pw);
          
          for (int i = 0; i < 2; i++)
          {
            if (WiFi.status() == WL_CONNECTED)
            {
              Log(LogCategoryWiFi, LogLevelDEBUG, "Reconnected to last successful network: ", 
                  String(configRTC.wificonfig[lastSuccessfulNetworkIndex].ssid));
              hasWifiConnection = true;
              synchronizeTimeWithNTP();
              return true;
            }
            delay(1000);
          }
          
          Log(LogCategoryWiFi, LogLevelDEBUG, "Reconnection to last network failed, trying all networks");
        }
      }
    }

    // If reconnection to last network failed or we have no last network,
    // search through all stored networks from top to bottom
    for (int j = 0; j < WifiArraySize; ++j)
    {
        
      if (strlen(configRTC.wificonfig[j].ssid) > 0)
      {
        Log(LogCategoryWiFi, LogLevelDEBUG, "Trying to connect to network: ", 
            String(configRTC.wificonfig[j].ssid));
        
        for (int attempt = 0; attempt < 2; ++attempt)
        {
          WiFi.begin(configRTC.wificonfig[j].ssid, configRTC.wificonfig[j].pw);
          
          for (int i = 0; i < 2; i++)
          {
            if (WiFi.status() == WL_CONNECTED)
            {
              Log(LogCategoryWiFi, LogLevelDEBUG, "Connected to network: ", 
                  String(configRTC.wificonfig[j].ssid));
              
              // Remember this successful network for next time
              lastSuccessfulNetworkIndex = j;
              
              hasWifiConnection = true;
              synchronizeTimeWithNTP();
              return true;
            }
            delay(1000);
          }

          Log(LogCategoryWiFi, LogLevelDEBUG, "Connection failed for network: ", 
              String(configRTC.wificonfig[j].ssid));
          hasWifiConnection = false;
        }
      }
    }

    Log(LogCategoryWiFi, LogLevelDEBUG, "No known network found");
    hasWifiConnection = false;
    return false;
  }
}

/**
 * @brief Handles NTP synchronization and LED indication.
 */
void handleNtpSynchronization()
{
  if (isNtpSynchronized)
  {
    bootFinishedLED();
  }
  else
  {
    bootFinishedButNtpUpdateNotPossibleLED();
  }
}