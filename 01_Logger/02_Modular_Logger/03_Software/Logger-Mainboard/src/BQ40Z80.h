#pragma once

#include <Arduino.h>
#include <stdint.h>

#include "init.h"

#ifndef BQ40Z80_ADDR
// Default 7-bit SMBus/I2C address per TRM is 0x16
#define BQ40Z80_ADDR 0x16
#endif

class BQ40Z80 {
public:
    // Manufacturer Block Access command code
    static constexpr uint8_t MBA_Command = 0x44;
    static constexpr uint16_t CMD_DeviceReset = 0x0041; // MAC: DeviceReset

    // Data-Flash addresses (per TRM)
    static constexpr uint16_t DF_ChargeTermTaperCurrent = 0x4CF3; // I2, mA
    static constexpr uint16_t DF_ChargeTermVoltage = 0x4CF7;      // I2, mV (delta)
    static constexpr uint16_t DF_SBSGaugingConfiguration = 0x4AC9; // H1, bit0=RSOCL

public:
    bool begin(uint8_t i2cAddr = BQ40Z80_ADDR);

    // Write helpers (UNSEALED/FULL ACCESS required for DF writes)
    bool writeDF_u8(uint16_t dfAddr, uint8_t value);
    bool writeDF_u16(uint16_t dfAddr, uint16_t value);

    // Read helpers (works SEALED for many addresses; may NACK if protected)
    bool readDF_u8(uint16_t dfAddr, uint8_t &value);
    bool readDF_u16(uint16_t dfAddr, uint16_t &value);

    // High-level setters
    bool setChargeTermination(uint16_t taper_mA, uint16_t deltaV_mV);
    bool setRSOCL(bool enable);

    // Dedicated setters for each requested tweak
    bool setChargeTermVoltage_mV(uint16_t deltaV_mV);           // DF_Charge Term Voltage: (0x4CF7)
    bool setChargeTermTaperCurrent_mA(uint16_t taper_mA);       // DF_Charge Term Taper Current (0x4CF3)
    bool enableRSOCL();                                         // SBS Gauging Configuration → RSOCL=1

    // One-shot convenience that applies all three (optional)
    bool applyRequestedTweaks(bool doReset = true);

    // Optional: soft reset via MAC 0x0041
    bool deviceReset();

private:
    bool readDF_raw(uint16_t dfAddr, uint8_t *out, uint8_t &outLen, uint8_t outCap);

    uint8_t _addr = BQ40Z80_ADDR;
};
