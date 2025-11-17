// src/bms_bq40_debug.cpp

#include <Arduino.h>
#include <Wire.h>
#include "init.h"           // BQ40Z80_ADDR, VOLTAGE_REG, Register_RSOC (from your project)
#include "DebuggingSDLog.h" // Shim: Log(LogCategory::..., LogLevel::..., ...)
#include "led.h"
#include "deepSleepService.h"

// ----------------- Possibly missing in init.h? Define here as a fallback -----------------
#ifndef CURRENT_REG
#define CURRENT_REG 0x0A // SBS: Instantaneous Current (signed, mA)
#endif
#ifndef AVG_CURRENT_REG
#define AVG_CURRENT_REG 0x0B // SBS: Average Current (signed, mA)
#endif

// HYFiVE uses the same block 0x71 for several fields (Cell_V, Cell_I, BAT/PACK, etc.) with offsets
#ifndef REGISTER_CELL_BLOCK
#define REGISTER_CELL_BLOCK 0x71
#endif

// Read N bytes starting at register 0x71 into buf[0..N-1]
static bool readReg71(uint8_t n, uint8_t *buf)
{
  Wire.beginTransmission(BQ40Z80_ADDR);
  Wire.write(REGISTER_CELL_BLOCK);
  if (Wire.endTransmission(false) != 0)
    return false; // repeated start
  uint8_t got = Wire.requestFrom((uint8_t)BQ40Z80_ADDR, n);
  if (got != n)
    return false;
  for (uint8_t i = 0; i < n; ++i)
    buf[i] = Wire.read();
  return true;
}

// Fetches all 4 cell voltages (mV) from 0x71
// HYFiVE-Mapping: Cell1_V -> buf[1..2], Cell2_V -> [3..4], Cell3_V -> [5..6], Cell4_V -> [7..8]
bool getAllCellVoltages_mV(uint16_t v_mV[4])
{
  uint8_t buf[10] = {0}; // we need at least up to index 8
  if (!readReg71(sizeof(buf), buf))
    return false;
  v_mV[0] = (uint16_t(buf[2]) << 8) | buf[1];
  v_mV[1] = (uint16_t(buf[4]) << 8) | buf[3];
  v_mV[2] = (uint16_t(buf[6]) << 8) | buf[5];
  v_mV[3] = (uint16_t(buf[8]) << 8) | buf[7];
  return true;
}

// Fetches all 4 cell currents (mA, signed) from 0x71
// HYFiVE-Mapping: Cell1_I -> buf[13..14], Cell2_I -> [15..16], Cell3_I -> [17..18], Cell4_I -> [19..20]
bool getAllCellCurrents_mA(int16_t i_mA[4])
{
  uint8_t buf[22] = {0}; // we need at least up to index 20
  if (!readReg71(sizeof(buf), buf))
    return false;
  i_mA[0] = (int16_t)((uint16_t(buf[14]) << 8) | buf[13]);
  i_mA[1] = (int16_t)((uint16_t(buf[16]) << 8) | buf[15]);
  i_mA[2] = (int16_t)((uint16_t(buf[18]) << 8) | buf[17]);
  i_mA[3] = (int16_t)((uint16_t(buf[20]) << 8) | buf[19]);
  return true;
}

namespace
{
  constexpr float kMinCellVoltageMv = 2850.0f; //! US18650VTC6 minimum (recommended shutdown): ≈ 2.5 V
  constexpr float kMaxCellVoltageMv = 4100.0f; //! US18650VTC6 maximum (voltage): 4.20 V charge termination (4.25 V absolute upper limit).
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
}

// Convenience function: reads both and logs like HYFiVE
void logAllCells_V_I()
{
  uint16_t v[4] = {0};
  int16_t i[4] = {0};

  if (getAllCellVoltages_mV(v))
  {
    Log(LogCategory::BMS, LogLevel::INFO,
        "CellV[mV] C1:", String(v[0]), " C2:", String(v[1]),
        " C3:", String(v[2]), " C4:", String(v[3]));
  }
  else
  {
    Log(LogCategory::BMS, LogLevel::WARN, "getAllCellVoltages_mV failed");
  }

  if (getAllCellCurrents_mA(i))
  {
    // Sign: usually + = charging current, - = discharge current (as in HYFiVE-Current) :contentReference[oaicite:3]{index=3}
    Log(LogCategory::BMS, LogLevel::INFO,
        "CellI[mA] C1:", String(i[0]), " C2:", String(i[1]),
        " C3:", String(i[2]), " C4:", String(i[3]));
  }
  else
  {
    Log(LogCategory::BMS, LogLevel::WARN, "getAllCellCurrents_mA failed");
  }
}

// ----------------- Configuration / Pins -----------------
static constexpr uint8_t PIN_STAT1 = 9;  // Charge status 1
static constexpr uint8_t PIN_STAT2 = 19; // Charge status 2
static constexpr uint8_t PIN_PG = 20;    // Power-good (charger present)

// Manufacturer Block Access + DeviceReset Command
static constexpr uint8_t MBA_Command = 0x44;
static constexpr uint16_t CMD_DeviceReset = 0x0041;

// ----------------- I2C helpers (internal only) -----------------
namespace
{
  // Read SBS word (little endian) -> uint16_t
  bool readWord_u16(uint8_t reg, uint16_t &out)
  {
    Wire.beginTransmission(BQ40Z80_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission() != 0)
      return false;
    if (Wire.requestFrom((uint8_t)BQ40Z80_ADDR, (uint8_t)2) != 2)
      return false;
    uint8_t lsb = Wire.read();
    uint8_t msb = Wire.read();
    out = (uint16_t)lsb | ((uint16_t)msb << 8);
    return true;
  }

  // Read SBS word (little endian) -> int16_t (for current/power-related signed values)
  bool readWord_i16(uint8_t reg, int16_t &out)
  {
    uint16_t raw = 0;
    if (!readWord_u16(reg, raw))
      return false;
    out = (int16_t)raw; // Interpret as two's complement
    return true;
  }

  // Temperature (Register 0x08, 0.1 K) -> °C
  bool readTempC(float &outC)
  {
    uint16_t raw = 0;
    if (!readWord_u16(0x08, raw))
      return false;
    outC = (raw / 10.0f) - 273.15f;
    return true;
  }

  // Remaining/Full Charge Capacity (0x0F/0x10) in mAh
  bool readCapacity(uint16_t reg, uint16_t &out)
  {
    return readWord_u16(reg, out);
  }

  // Piecewise-linear mapping from cell OCV -> SOC (%). Applies to REST voltage.
  float interpClamp(float x, const float *xs, const float *ys, int n)
  {
    if (x >= xs[0])
      return ys[0];
    if (x <= xs[n - 1])
      return ys[n - 1];
    for (int i = 0; i < n - 1; ++i)
    {
      if (x <= xs[i] && x >= xs[i + 1])
      {
        float t = (x - xs[i + 1]) / (xs[i] - xs[i + 1]);
        return ys[i + 1] + t * (ys[i] - ys[i + 1]);
      }
    }
    return ys[n - 1];
  }
}

// ----------------- Public helper functions -----------------

// SOC estimation from pack voltage (volt), assumption: 4S Li-Ion (11.2V .. 16.4V)
float estimateSocFromPackVoltage(float vpack_V)
{
  const float V_MIN = 11.2f; // 4*2.80 V/Z
  const float V_MAX = 16.4f; // 4*4.10 V/Z

  if (vpack_V <= V_MIN)
    return 0.0f;
  if (vpack_V >= V_MAX)
    return 100.0f;

  return (vpack_V - V_MIN) * (100.0f / (V_MAX - V_MIN));
}

// Convenient wrapper when only mV is available
inline float estimateSoc4S_from_mV(uint16_t pack_mV)
{
  return estimateSocFromPackVoltage(pack_mV / 1000.0f); // mV -> V
}

// --------- NEW: getCurrent (mA) and AverageCurrent (mA), both signed ---------
bool getCurrent_mA(int16_t &current_mA)
{
  // SBS 0x0A: Current, int16 (mA), + = charging, - = discharging
  if (!readWord_i16(CURRENT_REG, current_mA))
    return false;
  return true;
}

bool getAverageCurrent_mA(int16_t &avg_mA)
{
  // SBS 0x0B: AverageCurrent, int16 (mA)
  if (!readWord_i16(AVG_CURRENT_REG, avg_mA))
    return false;
  return true;
}

// BMS Device Reset via Manufacturer Block Access (0x44) + Subcommand 0x0041
bool bmsReset()
{
  Wire.beginTransmission(BQ40Z80_ADDR);
  Wire.write(MBA_Command);
  Wire.write(0x02);                            // Length 2 bytes
  Wire.write(uint8_t(CMD_DeviceReset & 0xFF)); // LSB
  Wire.write(uint8_t(CMD_DeviceReset >> 8));   // MSB
  uint8_t err = Wire.endTransmission();
  if (err == 0)
  {
    Log(LogCategory::BMS, LogLevel::INFO, "BMS DeviceReset (0x0041) sent");
    return true;
  }
  Log(LogCategory::BMS, LogLevel::WARN, "BMS DeviceReset failed, I2C err=", String(err));
  return false;
}

// One-time GPIO initialization for charger status pins
void ensureChargerPinsInit()
{
  static bool inited = false;
  if (inited)
    return;
  pinMode(PIN_STAT1, INPUT);
  pinMode(PIN_STAT2, INPUT);
  pinMode(PIN_PG, INPUT);
  inited = true;
}

// Output a snapshot of the key BMS data (including SOC estimate and currents)
void logBmsSnapshot()
{
  // Voltage
  uint16_t mV = 0;
  if (readWord_u16(VOLTAGE_REG, mV))
  {
    Log(LogCategory::BMS, LogLevel::INFO, "Vpack[mV]: ", String(mV));
    float soc_est = estimateSoc4S_from_mV(mV);
    Log(LogCategory::BMS, LogLevel::INFO, "SOC_est_from_V[%]: ", String(soc_est, 1));
  }
  else
  {
    Log(LogCategory::BMS, LogLevel::WARN, "Voltage read failed");
  }

  // RSOC
  uint16_t rsoc = 0;
  if (readWord_u16(Register_RSOC, rsoc))
  {
    Log(LogCategory::BMS, LogLevel::INFO, "Remaining[%]: ", String(rsoc));
  }
  else
  {
    Log(LogCategory::BMS, LogLevel::WARN, "Remaining[%] read failed");
  }

  // Current (mA), note the sign
  int16_t cur_mA = 0;
  if (getCurrent_mA(cur_mA))
  {
    Log(LogCategory::BMS, LogLevel::INFO, "Current[mA]: ", String(cur_mA));
  }
  else
  {
    Log(LogCategory::BMS, LogLevel::WARN, "Current read failed");
  }

  int16_t avg_mA = 0;
  if (getAverageCurrent_mA(avg_mA))
  {
    Log(LogCategory::BMS, LogLevel::DEBUG, "AvgCurrent[mA]: ", String(avg_mA));
  }

  // Internal temperature [°C]
  float tC = 0.f;
  if (readTempC(tC))
  {
    Log(LogCategory::BMS, LogLevel::INFO, "Temp.Int[degC]: ", String(tC, 2));
  }
  else
  {
    Log(LogCategory::BMS, LogLevel::WARN, "IntTemp read failed");
  }

  // Capacities
  uint16_t rem_mAh = 0, full_mAh = 0;
  if (readCapacity(0x0F, rem_mAh))
  {
    Log(LogCategory::BMS, LogLevel::INFO, "Capacity[mAh]: ", String(rem_mAh));
  }
  if (readCapacity(0x10, full_mAh))
  {
    Log(LogCategory::BMS, LogLevel::DEBUG, "FullChargeCapacity[mAh]: ", String(full_mAh));
  }

  // (Optional) Calculate power if voltage was available
  if (mV != 0 && cur_mA != 0)
  {
    // P[mW] = U[mV] * I[mA] / 1000   (or in W: /1'000'000)
    long p_mW = (long)mV * (long)cur_mA / 1000L;
    Log(LogCategory::BMS, LogLevel::DEBUG, "Power[mW]: ", String(p_mW));
  }
}

bool isPG()
{
  if (digitalRead(PIN_PG))
  {
    // Power supply not connected
    return true;
  }
  else
  {
    // Power supply connected
    return false;
  }
}
bool loadAktiv = 0;
// Read charger status and log briefly; also log voltage + SOC estimate
void manageBatteryCharging()
{

  while (!digitalRead(PIN_PG))
  {
    // bmsReset();
    // delay(500);

    int stat1 = digitalRead(PIN_STAT1);
    int stat2 = digitalRead(PIN_STAT2);
    int pg = digitalRead(PIN_PG);

    if (pg)
    {
      Log(LogCategory::BMS, LogLevel::INFO, "Charger PG=1 -> charging OFF");
    }
    else
    {
      Log(LogCategory::BMS, LogLevel::INFO, "Charger PG=0 -> charging ON");
    }
    Log(LogCategory::BMS, LogLevel::DEBUG, "STAT1:", String(stat1), " STAT2:", String(stat2), " PG:", String(pg));

    // Log voltage + SOC briefly
    uint16_t mV = 0;
    if (readWord_u16(VOLTAGE_REG, mV))
    {
      float soc_est = estimateSoc4S_from_mV(mV);
      Log(LogCategory::BMS, LogLevel::INFO, "Vpack[mV]: ", String(mV),
          "  SOC_est_from_V[%]: ", String(soc_est, 1));
    }
    else
    {
      Log(LogCategory::BMS, LogLevel::WARN, "Voltage read failed");
    }

    uint16_t cellVoltages[4] = {0};
    if (getAllCellVoltages_mV(cellVoltages))
    {
      uint16_t highestVoltage = 0;
      uint8_t highestIndex = 0;

      for (uint8_t cell = 0; cell < 4; ++cell)
      {
        float percent = cellVoltageToPercent(cellVoltages[cell]);
        Log(LogCategory::BMS, LogLevel::INFO,
            "CellV[mV] C", String(cell + 1), ": ", String(cellVoltages[cell]),
            " (", String(percent, 1), "%)");

        if (cellVoltages[cell] > highestVoltage)
        {
          highestVoltage = cellVoltages[cell];
          highestIndex = cell;
        }
      }

      float highestPercent = cellVoltageToPercent(highestVoltage);
      Log(LogCategory::BMS, LogLevel::INFO,
          "Highest cell voltage: C", String(highestIndex + 1), " = ",
          String(highestVoltage), "mV (", String(highestPercent, 1), "%)");
      ledBatteryPercent(highestPercent);
    }
    else
    {
      Log(LogCategory::BMS, LogLevel::WARN, "getAllCellVoltages_mV failed");
    }

    int16_t cellCurrents[4] = {0};
    if (getAllCellCurrents_mA(cellCurrents))
    {
      Log(LogCategory::BMS, LogLevel::INFO,
          "CellI[mA] C1:", String(cellCurrents[0]), " C2:", String(cellCurrents[1]),
          " C3:", String(cellCurrents[2]), " C4:", String(cellCurrents[3]));
      // if (cellCurrents[0] < 150 || cellCurrents[1] < 150 || cellCurrents[2] < 150 || cellCurrents[3] < 150)
      // {
      //   esp32Error();
      // }
    }
    else
    {
      Log(LogCategory::BMS, LogLevel::WARN, "getAllCellCurrents_mA failed");
    }

    delay(1000);

    loadAktiv = 1;
  }
  if (loadAktiv == 1)
  {
    ledResetAllStates();
    Serial.println("ledResetAllStates");
    loadAktiv = 0;
    initStatusLed();
  }
}
