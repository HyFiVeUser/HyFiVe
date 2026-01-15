/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Battery Management System (BMS) functionality
 */
#include "BMS.h"
#include "BMS_lib.h"
#include "DS3231TimeNtp.h"
#include "DebuggingSDLog.h"
#include "DeepSleep.h"
#include "LedManager.h"
#include "SDCard.h"
#include "SensorManagement.h"
#include "SystemVariables.h"
#include "Utility.h"
#include "WifiNetwork.h"

BMS_IC BMS;
TwoWire i2c(1); // TwoWire i2c(0);
uint8_t BufferLen = 64;
uint8_t i2cBuffer[64];
uint8_t BMS_address = 0xB;

constexpr float kMinCellVoltageMv   = 2850.0f; //! US18650VTC6 Minimum (empfohlen abschalten): ≈ 2.5V
constexpr float kMaxCellVoltageMv   = 4100.0f; //! US18650VTC6 Maximum (Spannung): 4.25V
constexpr float kCellVoltageRangeMv = kMaxCellVoltageMv - kMinCellVoltageMv;

float cellVoltageToPercent(uint16_t millivolts)
{
  if (kCellVoltageRangeMv <= 0.0f)
  {
    return 0.0f;
  }

  float percent = (static_cast<float>(millivolts) - kMinCellVoltageMv) * 100.0f / kCellVoltageRangeMv;

  if (percent < 0.0f)
  {
    percent = 0.0f;
  }
  else if (percent > 100.0f)
  {
    percent = 100.0f;
  }

  return percent;
}

/**
 * @brief Initializes the BMS and RTC.
 */
void initBmsAndRtc()
{
  BMS.Init(i2c, i2cBuffer, BufferLen, BMS_address); // BMS initialesieren
  initRTC(&i2c);                                    // DS3231 initialisieren
}

/**
 * @brief Checks if current of all cells is below 100mA
 * @return true if all cells are below 100mA, false otherwise
 */
bool getCellCurrent()
{
  if (BMS.getCell1_I() < 100 && BMS.getCell2_I() < 100 && BMS.getCell3_I() < 100 && BMS.getCell4_I() < 100)
  {
    return true;
  }
  else
  {
    return false;
  }
}

bool hasValidBmsSerialNumber()
{
  uint16_t serialNumber = BMS.getSerialNumber();

  if (serialNumber == 3)
  {
    Log(LogCategoryBMS, LogLevelINFO, "BMS Serial Number valid: ", String(serialNumber));
    return true;
  }
  else
  {
    Log(LogCategoryBMS, LogLevelINFO, "BMS Serial Number invalid: ", String(serialNumber));
    return false;
  }
}

/**
 * @brief Performs BMS reset
 */
void bmsReset()
{
  BMS.setRESET();
  Log(LogCategoryBMS, LogLevelDEBUG, "BMS RESET");
}

/**
 * @brief Gets the remaining battery percentage.
 * @return float The remaining battery percentage.
 */
// float getRemainingBatteryPercentage()
// {
//   float getRSOC = BMS.getRSOC();
//   return getRSOC;
// }

float getRemainingBatteryPercentage()
{
  const int stat1 = digitalRead(9);
  const int stat2 = digitalRead(19);
  const int pg    = digitalRead(20);

  const uint16_t cellVoltages[] = {BMS.getCell1_V(), BMS.getCell2_V(), BMS.getCell3_V(), BMS.getCell4_V()};
  const int16_t cellCurrents[]  = {BMS.getCell1_I(), BMS.getCell2_I(), BMS.getCell3_I(), BMS.getCell4_I()};
  float cellPercentages[4]      = {0.0f, 0.0f, 0.0f, 0.0f};

  size_t highestVoltageIndex = 0U;
  for (size_t index = 0; index < 4; ++index)
  {
    cellPercentages[index] = cellVoltageToPercent(cellVoltages[index]);
    if (cellVoltages[index] > cellVoltages[highestVoltageIndex])
    {
      highestVoltageIndex = index;
    }
  }

  const uint16_t packVoltage     = getTotalBatteryCellVoltage();
  const float packVoltageAverage = static_cast<float>(packVoltage) / 4.0f;
  const float socFromVoltage     = cellVoltageToPercent(static_cast<uint16_t>(packVoltageAverage));

  // Serial.print("[INFO] [BMS] STAT1:");
  // Serial.print(stat1);
  // Serial.print(" STAT2:");
  // Serial.print(stat2);
  // Serial.print(" PG:");
  // Serial.println(pg);
  //
  // Serial.print("[INFO] [BMS] Vpack[mV]: ");
  // Serial.print(packVoltage);
  // Serial.print("  SOC_est_from_V[%]: ");
  // Serial.println(socFromVoltage, 1);

  // for (size_t index = 0; index < 4; ++index)
  // {
  //   Serial.print("[INFO] [BMS] CellV[mV] C");
  //   Serial.print(index + 1);
  //   Serial.print(": ");
  //   Serial.print(cellVoltages[index]);
  //   Serial.print(" (");
  //   Serial.print(cellPercentages[index], 1);
  //   Serial.println("%)");
  // }

  // Serial.print("[INFO] [BMS] Maximum cell voltage: C");
  // Serial.print(highestVoltageIndex + 1);
  // Serial.print(" = ");
  // Serial.print(cellVoltages[highestVoltageIndex]);
  // Serial.print("mV (");
  // Serial.print(cellPercentages[highestVoltageIndex], 1);
  // Serial.println("%)");

  // Serial.print("[INFO] [BMS] CellI[mA] C1:");
  // Serial.print(cellCurrents[0]);
  // Serial.print(" C2:");
  // Serial.print(cellCurrents[1]);
  // Serial.print(" C3:");
  // Serial.print(cellCurrents[2]);
  // Serial.print(" C4:");
  // Serial.println(cellCurrents[3]);

  return cellPercentages[highestVoltageIndex];
}

/**
 * @brief Gets the total battery cell voltage.
 * @return uint16_t The total battery cell voltage in mV.
 */
uint16_t getTotalBatteryCellVoltage()
{
  uint16_t SumCellVoltage = BMS.getSumCellVoltage();
  return SumCellVoltage;
}

/**
 * @brief Gets the remaining battery capacity.
 * @return uint16_t The remaining battery capacity in mAh.
 */
uint16_t getRemainingBatteryCapacity()
{
  uint16_t RemainingCapacity = BMS.getRemainingCapacity();
  return RemainingCapacity;
}

void manageBatteryCharging()
{
  int counter                  = 0;
  int chargeTimer              = 0;
  int erroerBatteryCellVoltage = 0;
  int erreorBatteryPercentage  = 0;

  pinMode(9, INPUT);  // STAT1
  pinMode(19, INPUT); // STAT2
  pinMode(20, INPUT); // PG

  if (!digitalRead(20))
  {
    statusIsLoggerBusy.store(false);
  }

  for (int i = 0; i < 30; i++)
  {
    if ((BMS.getTS1Temp() / 10) - 273.15 == -273.15)
    {
      if (i == 29)
      {
        Log(LogCategoryBMS, LogLevelERROR, "BMS is not available");
        generalError();
      }
      // enable3V3();
      // disable3V3();
    }
    else
    {
      if (!bmsConfig)
      {
        Log(LogCategoryBMS, LogLevelINFO, "BMS ", "Log: ", "CellVoltage[mV]: ", String(getTotalBatteryCellVoltage()), " Remaining[%]: ", String(getRemainingBatteryPercentage()), " Capacity[mAh]: ", String(getRemainingBatteryCapacity()));
        Log(LogCategoryBMS, LogLevelINFO, "BMS ", "Log: ", "Temp. IntTemp[degC]: ", String((BMS.getIntTemp() / 10) - 273.15));
        Log(LogCategoryBMS, LogLevelINFO, "BMS ", "Log: ", "Temp. Battery[degC]: ", String((BMS.getTS1Temp() / 10) - 273.15));
        Log(LogCategoryBMS, LogLevelINFO, "BMS ", "Log: ", "getCell1_V: ", String(BMS.getCell1_V()), " getCell2_V: ", String(BMS.getCell2_V()), " getCell3_V: ", String(BMS.getCell3_V()), " getCell4_V: ", String(BMS.getCell4_V()));
        Log(LogCategoryBMS, LogLevelINFO, "BMS ", "Log: ", "getCell1_I: ", String(BMS.getCell1_I()), " getCell2_I: ", String(BMS.getCell2_I()), " getCell3_I: ", String(BMS.getCell3_I()), " getCell4_I: ", String(BMS.getCell4_I()));

        if (!hasValidBmsSerialNumber())
        {
          Serial.println("setBmsUndervoltageProtection");
          BMS.setBmsUndervoltageProtection();
        }
        bmsConfig = true;
      }
      break;
    }
  }

  if (BMS.getSafetyAlertAB() != 0 || BMS.getSafetyStatusAB() != 0 || BMS.getSafetyAlertCD() != 0 || BMS.getSafetyStatusCD() != 0)
  {
    Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "BMS Safety-Error");
    Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "CellVoltage[mV]: ", String(getTotalBatteryCellVoltage()), " Remaining[%]: ", String(getRemainingBatteryPercentage()), " Capacity[mAh]: ", String(getRemainingBatteryCapacity()));
    Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "Temp. IntTemp[degC]: ", String((BMS.getIntTemp() / 10) - 273.15));
    Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "Temp. Battery[degC]: ", String((BMS.getTS1Temp() / 10) - 273.15));
    Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "getCell1_V: ", String(BMS.getCell1_V()), " getCell2_V: ", String(BMS.getCell2_V()), " getCell3_V: ", String(BMS.getCell3_V()), " getCell4_V: ", String(BMS.getCell4_V()));
    Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "getCell1_I: ", String(BMS.getCell1_I()), " getCell2_I: ", String(BMS.getCell2_I()), " getCell3_I: ", String(BMS.getCell3_I()), " getCell4_I: ", String(BMS.getCell4_I()));
    Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "getSafetyAlertAB: ", String(BMS.getSafetyAlertAB()), " getSafetyStatusAB: ", String(BMS.getSafetyStatusAB()), " getSafetyAlertCD: ", String(BMS.getSafetyAlertCD()), " getSafetyStatusCD: ", String(BMS.getSafetyStatusCD()));
  }

  if (digitalRead(20))
  {
    float battery = getRemainingBatteryPercentage();

    if (battery <= 5)
    {
      ledControl(LedMode::batterySuperlow);
    }
    else if (battery <= 15)
    {
      ledControl(LedMode::batteryLow);
    }
  }

  if (!digitalRead(20))
  {
    BMS.setRESET();
    Log(LogCategoryPowerManagement, LogLevelINFO, "charging switched on");
    delay(3000);
  }

  if (!digitalRead(20))
  {
    for (int i = 0; i < 5; i++)
    {
      if (i == 4)
      {
        Log(LogCategoryBMS, LogLevelDEBUG, "BMS Safety-Error");
        // disable3V3();
        Log(LogCategoryPowerManagement, LogLevelINFO, "charging switched off");
        generalError();
      }
      if (BMS.getSafetyAlertAB() != 0 || BMS.getSafetyStatusAB() != 0 || BMS.getSafetyAlertCD() != 0 || BMS.getSafetyStatusCD() != 0)
      {
        Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "getSafetyAlertAB: ", String(BMS.getSafetyAlertAB()), " getSafetyStatusAB: ", String(BMS.getSafetyStatusAB()), " getSafetyAlertCD: ", String(BMS.getSafetyAlertCD()), " getSafetyStatusCD: ", String(BMS.getSafetyStatusCD()));
        BMS.setRESET();
      }
      else
      {
        Log(LogCategoryBMS, LogLevelDEBUG, "BMS OK");
        break;
      }
    }
  }

  while (!digitalRead(20))
  {
    counter++;
    chargeTimer++;

    Log(LogCategoryBMS, LogLevelDEBUG, "BMS ", "Log: ", "CellVoltage[mV]: ", String(getTotalBatteryCellVoltage()), " Remaining[%]: ", String(getRemainingBatteryPercentage()), " Capacity[mAh]: ", String(getRemainingBatteryCapacity()));
    Log(LogCategoryBMS, LogLevelDEBUG, "BMS ", "Log: ", "Temp. IntTemp[degC]: ", String((BMS.getIntTemp() / 10) - 273.15));
    Log(LogCategoryBMS, LogLevelDEBUG, "BMS ", "Log: ", "Temp. Battery[degC]: ", String((BMS.getTS1Temp() / 10) - 273.15));
    Log(LogCategoryBMS, LogLevelDEBUG, "BMS ", "Log: ", "getCell1_V: ", String(BMS.getCell1_V()), " getCell2_V: ", String(BMS.getCell2_V()), " getCell3_V: ", String(BMS.getCell3_V()), " getCell4_V: ", String(BMS.getCell4_V()));
    Log(LogCategoryBMS, LogLevelDEBUG, "BMS ", "Log: ", "getCell1_I: ", String(BMS.getCell1_I()), " getCell2_I: ", String(BMS.getCell2_I()), " getCell3_I: ", String(BMS.getCell3_I()), " getCell4_I: ", String(BMS.getCell4_I()));
    Log(LogCategoryBMS, LogLevelDEBUG, "BMS ", "Log: ", "getSafetyAlertAB: ", String(BMS.getSafetyAlertAB()), " getSafetyStatusAB: ", String(BMS.getSafetyStatusAB()), " getSafetyAlertCD: ", String(BMS.getSafetyAlertCD()), " getSafetyStatusCD: ", String(BMS.getSafetyStatusCD()));
    Log(LogCategoryCharger, LogLevelDEBUG, "STAT1: ", "Charge: ", String(digitalRead(9)));
    Log(LogCategoryCharger, LogLevelDEBUG, "STAT2: ", "Charge: ", String(digitalRead(19)));
    Log(LogCategoryCharger, LogLevelDEBUG, "PG: ", "Charge: ", String(digitalRead(20)));

    delay(1000);

    ledControl(LedMode::batteryCharging);

    for (int i = 0; i < 30; i++)
    {
      if (getTotalBatteryCellVoltage() < 10000 || getTotalBatteryCellVoltage() > 17000)
      {
        if (i == 20)
        {
          Log(LogCategoryBMS, LogLevelERROR, "BMS output incorrect");
          generalError();
        }
        delay(500);
      }
      else
      {
        break;
      }
    }

    if (BMS.getSafetyAlertAB() != 0 || BMS.getSafetyStatusAB() != 0 || BMS.getSafetyAlertCD() != 0 || BMS.getSafetyStatusCD() != 0)
    {
      Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "BMS Safety-Error");
      Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "getSafetyAlertAB: ", String(BMS.getSafetyAlertAB()), " getSafetyStatusAB: ", String(BMS.getSafetyStatusAB()), " getSafetyAlertCD: ", String(BMS.getSafetyAlertCD()), " getSafetyStatusCD: ", String(BMS.getSafetyStatusCD()));
      Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "CellVoltage[mV]: ", String(getTotalBatteryCellVoltage()), " Remaining[%]: ", String(getRemainingBatteryPercentage()), " Capacity[mAh]: ", String(getRemainingBatteryCapacity()));
      Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "Temp. IntTemp[degC]: ", String((BMS.getIntTemp() / 10) - 273.15));
      Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "Temp. Battery[degC]: ", String((BMS.getTS1Temp() / 10) - 273.15));
      Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "getCell1_V: ", String(BMS.getCell1_V()), " getCell2_V: ", String(BMS.getCell2_V()), " getCell3_V: ", String(BMS.getCell3_V()), " getCell4_V: ", String(BMS.getCell4_V()));
      Log(LogCategoryBMS, LogLevelERROR, "BMS ", "Log: ", "getCell1_I: ", String(BMS.getCell1_I()), " getCell2_I: ", String(BMS.getCell2_I()), " getCell3_I: ", String(BMS.getCell3_I()), " getCell4_I: ", String(BMS.getCell4_I()));
    }

    if (chargeTimer >= 600)
    {
      Serial.println("-------------------------------------------------0    24");

      Log(LogCategoryBMS, LogLevelINFO, "BMS ", "Log: ", "BMS INFO");
      Log(LogCategoryBMS, LogLevelINFO, "BMS ", "Log: ", "CellVoltage[mV]: ", String(getTotalBatteryCellVoltage()), " Remaining[%]: ", String(getRemainingBatteryPercentage()), " Capacity[mAh]: ", String(getRemainingBatteryCapacity()));
      Log(LogCategoryBMS, LogLevelINFO, "BMS ", "Log: ", "Temp. IntTemp[degC]: ", String((BMS.getIntTemp() / 10) - 273.15));
      Log(LogCategoryBMS, LogLevelINFO, "BMS ", "Log: ", "Temp. Battery[degC]: ", String((BMS.getTS1Temp() / 10) - 273.15));
      Log(LogCategoryBMS, LogLevelINFO, "BMS ", "Log: ", "getCell1_V: ", String(BMS.getCell1_V()), " getCell2_V: ", String(BMS.getCell2_V()), " getCell3_V: ", String(BMS.getCell3_V()), " getCell4_V: ", String(BMS.getCell4_V()));
      Log(LogCategoryBMS, LogLevelINFO, "BMS ", "Log: ", "getCell1_I: ", String(BMS.getCell1_I()), " getCell2_I: ", String(BMS.getCell2_I()), " getCell3_I: ", String(BMS.getCell3_I()), " getCell4_I: ", String(BMS.getCell4_I()));

      statusUploadPeriodeFunktion(0);
      processAndTransmitMeasurementData();

      chargeTimer = 0;
    }

    if (counter >= 60)
    {
      if (erroerBatteryCellVoltage == 30 || erreorBatteryPercentage == 30)
      {
        // disable3V3();
        Log(LogCategoryPowerManagement, LogLevelERROR, "charging switched off");
        Log(LogCategoryPowerManagement, LogLevelERROR, "Battery cannot be charged");
        generalError();
        return;
      }

      if ((getTotalBatteryCellVoltage() > 12000 && getTotalBatteryCellVoltage() < 16000) && BMS.getCurrent() < 500)
      {
        Log(LogCategoryBMS, LogLevelINFO, "BMS setRESET, BatteryCellVoltage, BMS.getCurrent() > 1000mA");
        erroerBatteryCellVoltage++;
        BMS.setRESET();
      }
      else
      {
        if (getRemainingBatteryPercentage() < 100 && BMS.getCurrent() < 50)
        {
          Log(LogCategoryBMS, LogLevelINFO, "BMS setRESET, BatteryPercentage, BMS.getCurrent() > 50mA");
          erreorBatteryPercentage++;
          BMS.setRESET();
        }
      }
      counter = 0;
    }

    if (!digitalRead(20))
    {
      if (BMS.getCell1_V() > 4100 || BMS.getCell2_V() > 4100 || BMS.getCell3_V() > 4100 || BMS.getCell4_V() > 4100)
      {
        Log(LogCategoryPowerManagement, LogLevelINFO, "battery completly charged");
        statusUploadPeriodeFunktion(0);
        while (!digitalRead(20))
        {
          statusLED = false;
          ledControl(LedMode::chargingComplete);
          while (!statusLED)
          {
            delay(10);
          };
          processAndTransmitMeasurementData();
        }
        return;
      }
    }
  }
}
