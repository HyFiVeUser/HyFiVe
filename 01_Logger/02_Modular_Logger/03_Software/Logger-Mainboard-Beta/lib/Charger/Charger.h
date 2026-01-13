/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Implementation of battery charger control functions
 */

#ifndef Charger_IC_H
#define Charger_IC_H_

enum ChargerState
{
  Charger_RSVD,
  Charger_CHG,
  Charger_CHG_Completed,
  Charger_CHG_Suspended,
  Charger_CHG_EN,
  Charger_Undefined_State,
};

class Charger_IC
{
public:
  Charger_IC();
  void Init();
  uint8_t getState();
  void setState(uint8_t Charge_State);
  void setCHG_Enable(uint8_t CE_State);
  uint8_t getCHG_Enable();
  uint8_t getPG();

private:
  uint8_t state;

  uint8_t STAT_1;
  uint8_t STAT_2;
  uint8_t PG;
  uint8_t CHG_Enable; // CE

  int STAT_1_PIN = 9;  // Status 1 Charger
  int STAT_2_PIN = 19; // Status 2 Charger
  int PG_PIN     = 20; // Power Good Charger
  int CE_Pin     = 10; // Charge Enable
};

void enableCharger();
void disableCharger();

#endif