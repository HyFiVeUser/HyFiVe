/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Firmware update management
 */

#include <ArduinoOTA.h>
#include <SD.h>
#include <WiFiClientSecure.h>

#include "firmwareUpdate.h"
#include "Led.h"
#include "SystemVariables.h"

/**
 * @brief Calculates the SHA-256 hash of the firmware file.
 */
String calculateCurrentHash()
{
  File file = SD.open("/updateFW/firmware.bin", FILE_READ);
  if (!file)
  {
    return "";
  }

  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  const size_t hash_size = 32;
  unsigned char hash[hash_size];

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);

  while (file.available())
  {
    char buf[128];
    int l = file.readBytes(buf, sizeof(buf));
    mbedtls_md_update(&ctx, (const unsigned char *)buf, l);
  }
  file.close();

  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  String hash_str = "";
  for (int i = 0; i < hash_size; i++)
  {
    char hex[3];
    sprintf(hex, "%02x", hash[i]);
    hash_str += hex;
  }
  return hash_str;
}

/**
 * @brief Updates the firmware from a file on the SD card.
 */
void updateFirmware()
{
  if (SD.exists("/updateFW/firmware.bin"))
  {
    File updateFile = SD.open("/updateFW/firmware.bin");
    if (updateFile)
    {
      if (Update.begin(updateFile.size()))
      {
        size_t written = Update.writeStream(updateFile);
        if (written == updateFile.size())
        {
          Serial.println("Firmware update successful!");

          File hashFile = SD.open("/updateFW/current_firmware.sha256", FILE_WRITE);
          if (hashFile)
          {
            String currentHash = calculateCurrentHash();
            hashFile.print(currentHash);
            hashFile.close();
            Serial.println("Hash saved to /updateFW/current_firmware.sha256");
          }
          else
          {
            Serial.println("Error saving hash file!");
          }
        }
        else
        {
          Serial.println("Firmware update failed!");
          SD.remove("/updateFW/firmware.bin");
        }
        if (Update.end())
        {
          if (Update.isFinished())
          {
            updateFile.close();
            SD.remove("/updateFW/firmware.bin");
            delay(1000);

            statusLED = false;
            ledControl(LedMode::startReboot);
            while (!statusLED)
            {
              delay(10);
            };

            ESP.restart();
          }
          else
          {
            Serial.println("Update not completed. Error!");
            SD.remove("/updateFW/firmware.bin");
          }
        }
        else
        {
          Serial.printf("Error when ending the update");
          SD.remove("/updateFW/firmware.bin");
        }
      }
      else
      {
        Serial.printf("Not enough memory for the update.");
        SD.remove("/updateFW/firmware.bin");
      }
      updateFile.close();
    }
    else
    {
      Serial.println("Error opening the firmware file.");
      SD.remove("/updateFW/firmware.bin");
    }
  }
}

/**
 * @brief Calculates and verifies SHA-256 hash of firmware file against provided hash
 * @param received_sha256 SHA-256 hash to verify against
 * @return true if hashes match, false if error or mismatch
 */
bool calculateSha256(String received_sha256)
{
  received_sha256.trim();

  if (SD.exists("/updateFW/firmware.bin"))
  {
    File file = SD.open("/updateFW/firmware.bin", FILE_READ);
    if (!file)
    {
      Serial.println("File could not be opened!");
      return false;
    }

    // Initialize the SHA-256 algorithm
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    const size_t hash_size = 32;
    unsigned char hash[hash_size];

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);

    // Read the file and calculate the hash
    while (file.available())
    {
      char buf[128];
      int l = file.readBytes(buf, sizeof(buf));
      mbedtls_md_update(&ctx, (const unsigned char *)buf, l);
    }
    file.close();

    mbedtls_md_finish(&ctx, hash);
    mbedtls_md_free(&ctx);

    String calculated_hash = "";
    for (int i = 0; i < hash_size; i++)
    {
      char hex[3];
      sprintf(hex, "%02x", hash[i]);
      calculated_hash += hex;
    }

    Serial.print("Calculated SHA-256: ");
    Serial.println(calculated_hash);
    Serial.print("Received SHA-256:  ");
    Serial.println(received_sha256);

    return (calculated_hash == received_sha256);
  }
  else
  {
    return false;
  }
}
