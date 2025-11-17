#include "firmware_updater.h"

#include "config.h"
#include "init.h"
#include "led.h"
#include "logging.h"

#include <HTTPClient.h>
#include <Update.h>
#include <WiFiClientSecure.h>
#include <esp_system.h>
#include <SD.h>
#include <algorithm>
#include <memory>

namespace
{
constexpr const char *CRC_URL =
    "https://raw.githubusercontent.com/Stanislas-Klein/AWI-FW/main/CRC32.txt";
constexpr const char *FIRMWARE_URL =
    "https://raw.githubusercontent.com/Stanislas-Klein/AWI-FW/main/firmware.bin";

constexpr const char *FIRMWARE_DIR = "/firmware_update";
constexpr const char *CRC_FILE_PATH = "/firmware_update/CRC32.txt";
constexpr const char *FIRMWARE_FILE_PATH = "/firmware_update/firmware.bin";

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = WIFI_TIMEOUT_MS;
constexpr uint32_t WIFI_RETRY_DELAY_MS = 500;

bool ensureFirmwareDirectory()
{
    if (SD.exists(FIRMWARE_DIR))
    {
        return true;
    }

    if (!SD.mkdir(FIRMWARE_DIR))
    {
        LOG_W("Could not create firmware update directory");
        return false;
    }

    return true;
}

String readStoredCrc()
{
    if (!SD.exists(CRC_FILE_PATH))
    {
        LOG_I("No local CRC32.txt found – firmware will be reloaded if needed");
        return String();
    }

    File crcFile = SD.open(CRC_FILE_PATH, FILE_READ);
    if (!crcFile)
    {
        LOG_W("Could not read CRC32.txt");
        return String();
    }

    String stored = crcFile.readString();
    crcFile.close();

    stored.trim();
    stored.toUpperCase();
    return stored;
}

void writeStoredCrc(const String &crc)
{
    if (!ensureFirmwareDirectory())
    {
        return;
    }

    if (SD.exists(CRC_FILE_PATH))
    {
        SD.remove(CRC_FILE_PATH);
    }

    File crcFile = SD.open(CRC_FILE_PATH, FILE_WRITE);
    if (!crcFile)
    {
        LOG_W("Could not write CRC32.txt");
        return;
    }

    crcFile.println(crc);
    crcFile.close();
}

bool ensureWifiConnected()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        return true;
    }

    const auto &ssid = systemConfig.mainMcu.wifiSsid;
    const auto &password = systemConfig.mainMcu.wifiPassword;

    if (ssid.isEmpty())
    {
        LOG_W("No WiFi configuration available – skipping firmware check");
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    LOG_I("Connecting for firmware update to '%s'...", ssid.c_str());

    const uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS)
    {
        delay(WIFI_RETRY_DELAY_MS);
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED)
    {
        LOG_E("WiFi connection for firmware update failed");
        WiFi.disconnect();
        return false;
    }

    LOG_I("✓ WiFi connected – starting firmware verification");
    return true;
}

bool fetchRemoteCrc(String &crcOut)
{
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15000);

    HTTPClient http;
    if (!http.begin(client, CRC_URL))
    {
        LOG_E("HTTP initialization for CRC failed");
        return false;
    }

    const int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        LOG_E("CRC32.txt could not be loaded (HTTP %d)", httpCode);
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    payload.trim();
    payload.toUpperCase();

    if (payload.isEmpty())
    {
        LOG_E("CRC32.txt was empty");
        return false;
    }

    crcOut = payload;
    LOG_I("CRC from repository: %s", crcOut.c_str());
    return true;
}

bool downloadAndInstallFirmware()
{
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(15000);

    HTTPClient http;
    LOG_I("Downloading firmware from %s", FIRMWARE_URL);
    if (!http.begin(client, FIRMWARE_URL))
    {
        LOG_E("HTTP initialization for firmware failed");
        return false;
    }

    const int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK)
    {
        LOG_E("Firmware could not be loaded (HTTP %d)", httpCode);
        http.end();
        return false;
    }

    const int contentLength = http.getSize();
    const bool canBegin = Update.begin(contentLength > 0 ? contentLength : UPDATE_SIZE_UNKNOWN);
    if (!canBegin)
    {
        LOG_E("Update.begin() failed");
        Update.printError(Serial);
        http.end();
        return false;
    }

    if (!ensureFirmwareDirectory())
    {
        Update.end();
        http.end();
        return false;
    }

    if (SD.exists(FIRMWARE_FILE_PATH))
    {
        SD.remove(FIRMWARE_FILE_PATH);
    }

    File firmwareFile = SD.open(FIRMWARE_FILE_PATH, FILE_WRITE);
    if (!firmwareFile)
    {
        LOG_E("Could not open firmware file for writing");
        Update.end();
        http.end();
        return false;
    }

    WiFiClient *stream = http.getStreamPtr();
    constexpr size_t BUFFER_SIZE = 4096;
    std::unique_ptr<uint8_t[]> buffer(new (std::nothrow) uint8_t[BUFFER_SIZE]);
    if (!buffer)
    {
        LOG_E("Memory for firmware buffer could not be allocated");
        firmwareFile.close();
        SD.remove(FIRMWARE_FILE_PATH);
        Update.end();
        http.end();
        return false;
    }

    size_t written = 0;
    while (http.connected() && (contentLength <= 0 || written < static_cast<size_t>(contentLength)))
    {
        size_t available = stream->available();
        if (!available)
        {
            delay(1);
            continue;
        }

        size_t toRead = std::min(BUFFER_SIZE, available);
        if (contentLength > 0)
        {
            toRead = std::min(toRead, static_cast<size_t>(contentLength) - written);
        }

        int bytesRead = stream->readBytes(buffer.get(), toRead);
        if (bytesRead <= 0)
        {
            LOG_E("Firmware download aborted (read error)");
            firmwareFile.close();
            SD.remove(FIRMWARE_FILE_PATH);
            Update.end();
            http.end();
            return false;
        }

        if (firmwareFile.write(buffer.get(), bytesRead) != static_cast<size_t>(bytesRead))
        {
            LOG_E("Writing the firmware file failed");
            firmwareFile.close();
            SD.remove(FIRMWARE_FILE_PATH);
            Update.end();
            http.end();
            return false;
        }

        size_t updateWritten = Update.write(buffer.get(), bytesRead);
        if (updateWritten != static_cast<size_t>(bytesRead))
        {
            LOG_E("Update.write() wrote fewer bytes than expected");
            firmwareFile.close();
            SD.remove(FIRMWARE_FILE_PATH);
            Update.end();
            http.end();
            return false;
        }

        written += bytesRead;
    }

    firmwareFile.close();

    if (contentLength > 0 && written != static_cast<size_t>(contentLength))
    {
        LOG_E("Firmware download incomplete (%u of %d bytes)", static_cast<unsigned>(written), contentLength);
        SD.remove(FIRMWARE_FILE_PATH);
        Update.end();
        http.end();
        return false;
    }

    if (!Update.end())
    {
        LOG_E("Update.end() reported an error");
        Update.printError(Serial);
        http.end();
        return false;
    }

    http.end();

    if (!Update.isFinished())
    {
        LOG_E("Update was not completed");
        return false;
    }

    LOG_I("Firmware installed successfully");
    return true;
}

} // namespace

bool performFirmwareUpdateIfAvailable()
{
    LOG_I("🔍 Checking for new firmware...");

    if (!ensureWifiConnected())
    {
        return false;
    }

    struct WiFiDisconnectGuard
    {
        ~WiFiDisconnectGuard()
        {
            WiFi.disconnect();
        }
    } guard;

    String remoteCrc;
    if (!fetchRemoteCrc(remoteCrc))
    {
        return false;
    }

    const String storedCrc = readStoredCrc();
    if (storedCrc == remoteCrc)
    {
        LOG_I("Firmware already up to date (CRC %s)", remoteCrc.c_str());
        return true;
    }

    LOG_I("🆕 New firmware detected (old: %s, new: %s)", storedCrc.c_str(), remoteCrc.c_str());

    if (!downloadAndInstallFirmware())
    {
        LOG_E("Firmware update failed");
        return false;
    }

    writeStoredCrc(remoteCrc);

    LOG_I("🔁 Restarting ESP32 after firmware update...");

    for (int i = 0; i < 20; ++i)
    {
        ledSignalMeasurement();
        delay(300); // Turn the LED on for 100 ms
    }

    delay(1000);
    ESP.restart();

    return true;
}

