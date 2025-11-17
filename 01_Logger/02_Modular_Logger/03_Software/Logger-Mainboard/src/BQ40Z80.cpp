// BQ40Z80.cpp
// Lightweight SMBus helper for TI BQ40Z80 data-flash tweaks
// Fits the OSBK_Nimbel code style (Wire + LOG_* macros)
//
// Implements:
// - Advanced Charge Algorithm → Termination Config
//   • Charge Term Voltage = 10 mV (addr 0x4CF7)
//   • Charge Term Taper Current ≈ 200 mA (addr 0x4CF3)
// - SBS Gauging Configuration (addr 0x4AC9) → set RSOCL bit (0x04 → 0x05)
// - Optional DeviceReset() to apply changes
//
// Notes:
// • Writes require UNSEALED or FULL ACCESS mode on the gauge.
// • ManufacturerBlockAccess (0x44) uses SMBus block protocol with a byte-count prefix.
// • All multi-byte values are little-endian when sent to 0x44.
// • Safe to call applyRequestedTweaks() repeatedly; it re-writes idempotent values.

#include "BQ40Z80.h"

#include <Wire.h>

#ifndef BQ40Z80_ADDR
// Default 7-bit SMBus/I2C address per TRM is 0x16
#define BQ40Z80_ADDR 0x16
#endif

// -------- Local logging fallbacks (if project logging.h not included before this) --------
#ifndef LOG_I
# define LOG_I(fmt, ...) do { Serial.printf("[BQ40Z80] " fmt "\n", ##__VA_ARGS__); } while (0)
# define LOG_W(fmt, ...) do { Serial.printf("[BQ40Z80][W] " fmt "\n", ##__VA_ARGS__); } while (0)
# define LOG_E(fmt, ...) do { Serial.printf("[BQ40Z80][E] " fmt "\n", ##__VA_ARGS__); } while (0)
#endif

namespace
{
    static bool smbusBlockWrite(uint8_t i2cAddr, uint8_t cmd, const uint8_t *data, uint8_t len)
    {
        Wire.beginTransmission(i2cAddr);
        Wire.write(cmd);     // command (e.g. 0x44)
        Wire.write(len);     // SMBus byte count
        Wire.write(data, len); // payload
        const auto err = Wire.endTransmission();
        if (err != 0)
        {
            LOG_E("blockWrite cmd=0x%02X len=%u -> I2C err=%d", cmd, len, err);
            return false;
        }
        return true;
    }

    static bool smbusBlockRead(uint8_t i2cAddr, uint8_t cmd, uint8_t *out, uint8_t &outLen, uint8_t outCap)
    {
        if (outCap == 0)
        {
            outLen = 0;
            return false;
        }

        // Issue command without releasing the bus
        Wire.beginTransmission(i2cAddr);
        Wire.write(cmd);
        if (Wire.endTransmission(false) != 0)
        {
            LOG_E("blockRead cmd=0x%02X write phase failed", cmd);
            return false;
        }

        const uint8_t req = outCap + 1; // include the count byte
        uint8_t got = Wire.requestFrom(static_cast<int>(i2cAddr), static_cast<int>(req));
        if (got < 1)
        {
            LOG_E("blockRead cmd=0x%02X no data", cmd);
            return false;
        }

        const uint8_t n = Wire.read(); // byte count supplied by gauge
        if (n > outCap)
        {
            LOG_W("blockRead count=%u > cap=%u; truncating", n, outCap);
        }

        const uint8_t toCopy = (n <= outCap) ? n : outCap;
        for (uint8_t i = 0; i < toCopy && Wire.available(); ++i)
        {
            out[i] = Wire.read();
        }
        // Drain any unused bytes to keep Wire state consistent
        while (Wire.available())
        {
            (void)Wire.read();
        }

        outLen = toCopy;
        return true;
    }
}

bool BQ40Z80::begin(uint8_t i2cAddr)
{
    _addr = i2cAddr;

    Wire.beginTransmission(_addr);
    uint8_t err = Wire.endTransmission();
    if (err != 0)
    {
        LOG_E("begin(): gauge not responding at 0x%02X (err=%u)", _addr, err);
        return false;
    }

    LOG_I("Gauge detected at 0x%02X", _addr);
    return true;
}

bool BQ40Z80::writeDF_u8(uint16_t dfAddr, uint8_t value)
{
    uint8_t payload[3] = {
        static_cast<uint8_t>(dfAddr & 0xFF),
        static_cast<uint8_t>((dfAddr >> 8) & 0xFF),
        value};

    if (!smbusBlockWrite(_addr, MBA_Command, payload, sizeof(payload)))
    {
        LOG_E("writeDF_u8 addr=0x%04X failed", dfAddr);
        return false;
    }

    delay(5);
    return true;
}

bool BQ40Z80::writeDF_u16(uint16_t dfAddr, uint16_t value)
{
    uint8_t payload[4] = {
        static_cast<uint8_t>(dfAddr & 0xFF),
        static_cast<uint8_t>((dfAddr >> 8) & 0xFF),
        static_cast<uint8_t>(value & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF)};

    if (!smbusBlockWrite(_addr, MBA_Command, payload, sizeof(payload)))
    {
        LOG_E("writeDF_u16 addr=0x%04X failed", dfAddr);
        return false;
    }

    delay(5);
    return true;
}

bool BQ40Z80::readDF_raw(uint16_t dfAddr, uint8_t *out, uint8_t &outLen, uint8_t outCap)
{
    if (out == nullptr || outCap == 0)
    {
        outLen = 0;
        return false;
    }

    uint8_t requested = outLen;
    if (requested == 0 || requested > outCap)
    {
        requested = outCap;
    }

    uint8_t payload[3] = {
        static_cast<uint8_t>(dfAddr & 0xFF),
        static_cast<uint8_t>((dfAddr >> 8) & 0xFF),
        requested};

    if (!smbusBlockWrite(_addr, MBA_Command, payload, sizeof(payload)))
    {
        LOG_E("readDF_raw addr=0x%04X write phase failed", dfAddr);
        outLen = 0;
        return false;
    }

    delay(5);

    uint8_t len = requested;
    if (!smbusBlockRead(_addr, MBA_Command, out, len, outCap))
    {
        outLen = 0;
        return false;
    }

    outLen = len;
    return true;
}

bool BQ40Z80::readDF_u8(uint16_t dfAddr, uint8_t &value)
{
    uint8_t len = 1;
    if (!readDF_raw(dfAddr, &value, len, 1) || len != 1)
    {
        return false;
    }
    return true;
}

bool BQ40Z80::readDF_u16(uint16_t dfAddr, uint16_t &value)
{
    uint8_t buf[2] = {};
    uint8_t len = 2;
    if (!readDF_raw(dfAddr, buf, len, sizeof(buf)) || len != 2)
    {
        return false;
    }

    value = static_cast<uint16_t>(buf[0]) | (static_cast<uint16_t>(buf[1]) << 8);
    return true;
}

bool BQ40Z80::setChargeTermination(uint16_t taper_mA, uint16_t deltaV_mV)
{
    bool ok = true;
    if (!setChargeTermTaperCurrent_mA(taper_mA))
    {
        ok = false;
    }
    if (!setChargeTermVoltage_mV(deltaV_mV))
    {
        ok = false;
    }
    return ok;
}

bool BQ40Z80::setRSOCL(bool enable)
{
    const uint8_t newValue = 5;
    return writeDF_u8(DF_SBSGaugingConfiguration, newValue);
}

bool BQ40Z80::setChargeTermVoltage_mV(uint16_t deltaV_mV)
{
    return writeDF_u16(DF_ChargeTermVoltage, deltaV_mV);
}

bool BQ40Z80::setChargeTermTaperCurrent_mA(uint16_t taper_mA)
{
    return writeDF_u16(DF_ChargeTermTaperCurrent, taper_mA);
}

bool BQ40Z80::enableRSOCL()
{
    return setRSOCL(true);
}

bool BQ40Z80::applyRequestedTweaks(bool doReset)
{
    bool ok = true;

    if (!setChargeTermVoltage_mV(10))
    {
        ok = false;
    }

    if (!setChargeTermTaperCurrent_mA(200))
    {
        ok = false;
    }

    if (!enableRSOCL())
    {
        ok = false;
    }

    if (doReset)
    {
        if (!deviceReset())
        {
            ok = false;
        }
        else
        {
            delay(100);
        }
    }

    return ok;
}

bool BQ40Z80::deviceReset()
{
    const uint16_t cmd = CMD_DeviceReset;
    uint8_t payload[2] = {
        static_cast<uint8_t>(cmd & 0xFF),
        static_cast<uint8_t>((cmd >> 8) & 0xFF)};

    return smbusBlockWrite(_addr, MBA_Command, payload, sizeof(payload));
}
