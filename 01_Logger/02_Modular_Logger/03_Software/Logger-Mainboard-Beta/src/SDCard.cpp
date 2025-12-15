/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: SD card initialization and management
 */

#include <SD.h>

#include "Led.h"
#include "SDCard.h"

SPIClass initSPIclass;

const uint8_t csPin = 18; // Chip Select Pin

void initializeSpi()
{
  uint8_t sckPin = 2;   // Clock Pin
  uint8_t misoPin = 38; // Master In Slave Out Pin
  uint8_t mosiPin = 1;  // Master Out Slave In Pin
  initSPIclass.begin(sckPin, misoPin, mosiPin, csPin);
}

/**
 * @brief Initializes the SD card.
 * @return bool True if initialization was successful, false otherwise.
 */
bool initializeSdCard()
{

  initializeSpi();
  delay(50);
  if (SD.begin(csPin, initSPIclass, 80000000))
  {
    // Serial.println("SD card initialized successfully.");
    return true;
  }
  else
  {
    generalError();
    return false;
  }
}
