#include "ble_config.h"
#include "ble_server.h"
#include "init.h"
#include "logging.h"
#include "power.h"
#include <freertos/queue.h>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <vector>

// =========================
// GLOBAL VARIABLES
// =========================

bool bleConfigUploadActive = false;
static bool serviceInitialized = false;
static NimBLEService *configService = nullptr;
static NimBLECharacteristic *configCharacteristic = nullptr;
static ConfigurationReceiver *configurationReceiver = nullptr;
static unsigned long lastChunkTimestamp = 0;

static constexpr unsigned long CONFIG_UPLOAD_TIMEOUT_MS = 5000;

static size_t computeConfigPayloadCapacity()
{
    constexpr size_t headerSize = sizeof(((ConfigTransferPacket *)nullptr)->metadata) +
                                  sizeof(((ConfigTransferPacket *)nullptr)->chunk.index) +
                                  sizeof(((ConfigTransferPacket *)nullptr)->chunk.checksum) +
                                  sizeof(((ConfigTransferPacket *)nullptr)->chunk.payload_size);

    constexpr size_t structCapacity = sizeof(((ConfigTransferPacket *)nullptr)->chunk.payload);

    const uint16_t mtu = NimBLEDevice::getMTU();
    const size_t attBytes = (mtu > 3) ? static_cast<size_t>(mtu - 3) : 0;

    if (attBytes <= headerSize)
    {
        return 0;
    }

    const size_t available = attBytes - headerSize;
    return (available < structCapacity) ? available : structCapacity;
}

// Packet receiver for chunked transfer
struct ConfigPacketAssembler
{
    uint16_t expectedTotalPackets = 0;
    uint16_t receivedChunkCount = 0;
    String collectedContent;
    std::vector<String> chunkBuffer;
    std::vector<bool> chunkReceived;

    void reset()
    {
        expectedTotalPackets = 0;
        receivedChunkCount = 0;
        collectedContent.clear();
        chunkBuffer.clear();
        chunkReceived.clear();
    }

    bool appendPacket(const ConfigTransferPacket &packet)
    {
        const uint16_t totalPackets = packet.metadata.total_num_packets;
        const uint16_t chunkIndex = packet.chunk.index;

        if (totalPackets == 0)
        {
            LOG_W("Chunk with totalPackets=0 received – ignoring");
            return false;
        }

        if (expectedTotalPackets == 0)
        {
            expectedTotalPackets = totalPackets;
            chunkBuffer.assign(expectedTotalPackets, String());
            chunkReceived.assign(expectedTotalPackets, false);
        }
        else if (expectedTotalPackets != totalPackets)
        {
            LOG_W("Inconsistent number of chunks (expected %u, received %u) – restarting", expectedTotalPackets, totalPackets);
            reset();
            expectedTotalPackets = totalPackets;
            chunkBuffer.assign(expectedTotalPackets, String());
            chunkReceived.assign(expectedTotalPackets, false);
        }

        if (chunkIndex >= expectedTotalPackets)
        {
            LOG_W("Chunk index %u outside the expected range (0-%u) – ignoring", chunkIndex, expectedTotalPackets - 1);
            return false;
        }

        if (chunkReceived[chunkIndex])
        {
            LOG_W("Chunk %u already received – ignoring duplicate", chunkIndex);
            return (receivedChunkCount == expectedTotalPackets);
        }

        String payload;
        if (packet.chunk.payload_size > 0)
        {
            payload.reserve(packet.chunk.payload_size);
            payload.concat((const char *)packet.chunk.payload, packet.chunk.payload_size);
        }

        chunkBuffer[chunkIndex] = payload;
        chunkReceived[chunkIndex] = true;
        receivedChunkCount++;

        if (receivedChunkCount == expectedTotalPackets)
        {
            size_t totalLength = 0;
            for (const auto &chunk : chunkBuffer)
            {
                totalLength += chunk.length();
            }

            collectedContent = "";
            collectedContent.reserve(totalLength);
            for (const auto &chunk : chunkBuffer)
            {
                collectedContent += chunk;
            }

            return true;
        }

        return false;
    }
};

static ConfigPacketAssembler configPacketAssembler;

static bool deletePathRecursive(const char *path)
{
    File entry = SD.open(path);
    if (!entry)
    {
        LOG_W("Could not open path '%s'", path);
        return false;
    }

    if (!entry.isDirectory())
    {
        entry.close();
        if (!SD.remove(path))
        {
            LOG_W("Could not remove file '%s'", path);
            return false;
        }
        return true;
    }

    bool success = true;
    while (true)
    {
        File child = entry.openNextFile();
        if (!child)
        {
            break;
        }

        String childPath = String(path);
        if (!childPath.endsWith("/"))
        {
            childPath += "/";
        }
        childPath += child.name();
        child.close();

        if (!deletePathRecursive(childPath.c_str()))
        {
            success = false;
        }
    }

    entry.close();

    if (!SD.rmdir(path))
    {
        LOG_W("Could not remove directory '%s'", path);
        return false;
    }

    return success;
}

static void clearConfigUpdateDirectory(const char *directoryPath)
{
    if (!SD.exists(directoryPath))
    {
        if (SD.mkdir(directoryPath))
        {
            LOG_I("📁 Directory '%s' created", directoryPath);
        }
        else
        {
            LOG_W("Directory '%s' could not be created", directoryPath);
        }
        return;
    }

    File dir = SD.open(directoryPath);
    if (!dir)
    {
        LOG_W("Directory '%s' could not be opened", directoryPath);
        return;
    }

    if (!dir.isDirectory())
    {
        LOG_W("Path '%s' is not a directory", directoryPath);
        dir.close();
        return;
    }

    bool success = true;
    while (true)
    {
        File entry = dir.openNextFile();
        if (!entry)
        {
            break;
        }

        String entryPath = String(directoryPath);
        if (!entryPath.endsWith("/"))
        {
            entryPath += "/";
        }
        entryPath += entry.name();
        entry.close();

        if (!deletePathRecursive(entryPath.c_str()))
        {
            success = false;
        }
    }

    dir.close();

    if (success)
    {
        LOG_I("Contents of '%s' were deleted", directoryPath);
    }
    else
    {
        LOG_W("Contents of '%s' could not be fully deleted", directoryPath);
    }
}

static void clearUploadState()
{
    configPacketAssembler.reset();
    lastChunkTimestamp = 0;
}

void resetConfigUploadState()
{
    LOG_W("resetConfigUploadState");
    bleConfigUploadActive = false;
    clearUploadState();
}

// =========================
// CHARACTERISTIC CALLBACKS
// =========================
class ConfigCharacteristicCallbacks : public NimBLECharacteristicCallbacks
{
public:
    void onWrite(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        std::string value = pCharacteristic->getValue();

        if (value.length() == sizeof(ConfigTransferPacket))
        {
            ConfigTransferPacket *packet = (ConfigTransferPacket *)value.data();

            // Mark the start of the upload
            if (packet->chunk.index == 0)
            {
                configPacketAssembler.reset();
                bleConfigUploadActive = true;
                LOG_I("Config upload started");
                if (requestType == "")
                {
                    clearConfigUpdateDirectory(CONFIG_UPDATE_DIR);
                }
            }
            else if (!bleConfigUploadActive)
            {
                LOG_W("Chunk received without active upload (index %u) - discarding", packet->chunk.index);
                return;
            }

            if (bleConfigUploadActive)
            {
                lastChunkTimestamp = millis();
            }

            // Process the packet
            if (configPacketAssembler.appendPacket(*packet))
            {
                LOG_I("Config transfer complete");

                // Forward to the ConfigurationReceiver
                if (configurationReceiver)
                {
                    configurationReceiver->processReceivedData(
                        (const uint8_t *)configPacketAssembler.collectedContent.c_str(),
                        configPacketAssembler.collectedContent.length());

                    // Send response
                    ConfigResponse responseCode = configurationReceiver->getLastResponse();
                    String response = configurationReceiver->generateProperResponse(responseCode);
                    sendConfigResponse(response, pCharacteristic, connInfo);
                }

                // Finish upload
                clearUploadState();
                LOG_I("requestType %s", requestType);

                if (configurationReceiver && configurationReceiver->getLastCommand() == CMD_NEW_CONFIG)
                {
                    getContentJson();
                    bleConfigUploadActive = false;
                }

                LOG_I("Config upload finished");
                bleConfigUploadActive = false;
            }
        }
    }

    void onRead(NimBLECharacteristic *pCharacteristic, NimBLEConnInfo &connInfo) override
    {
        pCharacteristic->setValue("ESP32-S3 Config Service");
    }

private:
    static uint16_t computeTransferChecksum(const uint8_t * /*data*/, size_t /*length*/)
    {
        // The current desktop tool implementation expects a constant 0xDEAD as
        // the checksum. Once both the tool and firmware switch to a real CRC-16,
        // this function can be adjusted accordingly.
        return 0xDEAD;
    }

    static void sendConfigResponse(const String &response, NimBLECharacteristic *characteristic, NimBLEConnInfo &connInfo)
    {
        const size_t payloadCapacity = computeConfigPayloadCapacity();

        if (payloadCapacity == 0)
        {
            LOG_W("Config response cannot be sent – MTU too small");
            return;
        }
        const size_t totalLength = response.length();
        const uint8_t *responseBytes = reinterpret_cast<const uint8_t *>(response.c_str());

        const uint16_t totalPackets = static_cast<uint16_t>(
            totalLength == 0 ? 1 : (totalLength + payloadCapacity - 1) / payloadCapacity);
        const uint16_t wholeChecksum = computeTransferChecksum(responseBytes, totalLength);

        for (uint16_t index = 0; index < totalPackets; ++index)
        {
            ConfigTransferPacket pkt{};
            pkt.metadata.total_num_packets = totalPackets;
            pkt.metadata.whole_packet_checksum = wholeChecksum;
            pkt.chunk.index = index;

            const size_t offset = static_cast<size_t>(index) * payloadCapacity;
            const size_t remaining = (offset < totalLength) ? (totalLength - offset) : 0;
            const size_t chunkSize = remaining > payloadCapacity ? payloadCapacity : remaining;

            pkt.chunk.payload_size = static_cast<uint16_t>(chunkSize);
            pkt.chunk.checksum = computeTransferChecksum(responseBytes + offset, chunkSize);

            if (chunkSize > 0)
            {
                std::memcpy(pkt.chunk.payload, responseBytes + offset, chunkSize);
            }

            const size_t dynamicLength = sizeof(pkt.metadata) +
                                         sizeof(pkt.chunk.index) +
                                         sizeof(pkt.chunk.checksum) +
                                         sizeof(pkt.chunk.payload_size) +
                                         chunkSize;

            characteristic->setValue(reinterpret_cast<uint8_t *>(&pkt), dynamicLength);

            bool sent = false;
            const uint16_t connHandle = connInfo.getConnHandle();
            for (uint8_t attempt = 0; attempt < 5 && !sent; ++attempt)
            {
                sent = characteristic->notify(connHandle);

                if (!sent)
                {
                    sent = characteristic->notify();
                }

                if (!sent)
                {
                    vTaskDelay(pdMS_TO_TICKS(10));
                }
            }

            if (!sent)
            {
                LOG_W("Config response notify failed (chunk %u)", static_cast<unsigned>(index));
                break;
            }

            vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
};

static ConfigCharacteristicCallbacks *configCharacteristicCallbacks = nullptr;

// =========================
// MAIN FUNCTIONS
// =========================
bool initConfigService()
{
    if (serviceInitialized)
    {
        LOG_W("Config service already initialized");
        return true;
    }

    if (!bleServerInitialized || !bleServerInstance)
    {
        LOG_E("BLE server not available");
        return false;
    }

    LOG_I(" Initializing config service...");

    try
    {
        // Service erstellen
        configService = bleServerInstance->createService(CONFIG_SERVICE_UUID);
        if (!configService)
        {
            LOG_E("Config service could not be created");
            return false;
        }

        // Create characteristic
        configCharacteristic = configService->createCharacteristic(
            CONFIG_CHARACTERISTIC_UUID,
            NIMBLE_PROPERTY::READ |
                NIMBLE_PROPERTY::WRITE |
                NIMBLE_PROPERTY::NOTIFY |
                NIMBLE_PROPERTY::INDICATE);

        if (!configCharacteristic)
        {
            LOG_E("Config characteristic could not be created");
            return false;
        }

        // Set callbacks
        configCharacteristicCallbacks = new ConfigCharacteristicCallbacks();
        configCharacteristic->setCallbacks(configCharacteristicCallbacks);

        // Descriptor for indications
        configCharacteristic->createDescriptor("2902");

        // Default value
        configCharacteristic->setValue("ESP32-S3 Config Service");

        // Create configuration receiver
        configurationReceiver = new ConfigurationReceiver();

        serviceInitialized = true;
        LOG_I("Config service initialized");
        return true;
    }
    catch (const std::exception &e)
    {
        LOG_E("Config service error: %s", e.what());
        return false;
    }
}

bool startConfigService()
{
    if (!serviceInitialized)
    {
        LOG_E("Config service not initialized");
        return false;
    }

    try
    {
        configService->start();
        LOG_I("Config service started");
        return true;
    }
    catch (const std::exception &e)
    {
        LOG_E("Config service start failed: %s", e.what());
        return false;
    }
}

void stopConfigService()
{
    // Service cannot truly be stopped
    resetConfigUploadState();
    LOG_W("Config service stopped");
}

void pollConfigUploadTimeout()
{
    if (!bleConfigUploadActive)
    {
        return;
    }

    if (!isBLEDeviceConnected())
    {
        LOG_W("Config upload aborted: connection lost");
        clearUploadState();
        bleConfigUploadActive = false;
        return;
    }

    if (lastChunkTimestamp == 0)
    {
        return;
    }

    const unsigned long elapsed = millis() - lastChunkTimestamp;
    if (elapsed > CONFIG_UPLOAD_TIMEOUT_MS)
    {
        LOG_W("Config upload timeout after %lu ms", static_cast<unsigned long>(elapsed));
        clearUploadState();
        bleConfigUploadActive = false;
    }
}


bool isConfigUploadActive()
{
    return bleConfigUploadActive;
}

void printConfigServiceStatus()
{
    LOG_I("\n === CONFIG SERVICE STATUS ===");
    LOG_I("Service initialized: %s", serviceInitialized ? "✅" : "❌");
    LOG_I("Config upload active: %s", bleConfigUploadActive ? "🔄" : "⏸️");
    LOG_I("===============================\n");
}
