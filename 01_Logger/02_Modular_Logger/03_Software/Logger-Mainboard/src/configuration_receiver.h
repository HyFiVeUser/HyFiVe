#ifndef CONFIGURATION_RECEIVER_H
#define CONFIGURATION_RECEIVER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SD.h>
#include "ble_uuids.h"

// BT-CFG protocol commands (from the app documentation)
enum ConfigCommand : uint8_t {
    CMD_LIST_FILES = 0x00,  // LS - list files
    CMD_GET_FILE = 0x01,    // FG - file get
    CMD_SAVE_FILE = 0x02,   // FS - file save
    CMD_NEW_CONFIG = 0x03   // C - new config
};

enum ConfigResponse : uint8_t {
    RESP_OK = 0x00,                // Acknowledge without an error
    RESP_UNKNOWN_COMMAND = 0x01,   // the command-field was invalid
    RESP_JSON_ERROR = 0x02,        // the json-parser had a problem with the sent config
    RESP_INVALID_CONFIG = 0x03,    // placeholder for when we check the config-content
    RESP_FILE_NOT_FOUND = 0x04     // if we couldn't find the requested file
};

void getContentJson();

class ConfigurationReceiver {
private:
    // Packet reassembly
    String receivedData;
    bool receivingPacket;
    size_t expectedPacketSize;
    size_t currentPacketSize;
    
    // Statistics
    int filesReceived;
    int configurationsReceived;
    
    // Store last command for proper response
    ConfigCommand lastCommand;
    ConfigResponse lastResponse;

    // Helper functions
    String extractFilename(const String& data, int maxFilenameSize = 100);
    void parseCommand(const String& data);
    void handleFileCommand(const String& data);
    void handleConfigCommand(const String& data);
    void logReceivedFile(const String& filename, const String& content);
    void logReceivedConfig(const String& configJson);
    String resolveFilePath(const String& filename);
    bool ensureDirectoryExists(const String& directoryPath);
    bool tryHandleChunkedTransfer(const uint8_t* data, size_t length, uint8_t headerByte);
    String buildFileListJson();
    bool loadFileContent(const String& filename, String& content);

public:
    ConfigurationReceiver();

    // Main function - processes received data
    void processReceivedData(const uint8_t* data, size_t length);
    
    // Statistics
    void printStatistics();
    void reset();

    // DEPRECATED: Old response function (for compatibility)
    String generateResponse(ConfigResponse response, const String& data = "");

    String generateProperResponse(ConfigResponse response, const String& data = "");

    // Get last command for response
    ConfigCommand getLastCommand() const { return lastCommand; }
    ConfigResponse getLastResponse() const { return lastResponse; }

private:
    String pendingResponseData;
};

#endif