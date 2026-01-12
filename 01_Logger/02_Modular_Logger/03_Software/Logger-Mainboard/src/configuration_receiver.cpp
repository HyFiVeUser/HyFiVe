#include "configuration_receiver.h"
#include "init.h"
#include "logging.h"
#include "sensors.h"
#include "power.h"
#include "measurement_storage.h"
#include <algorithm>
#include <vector>

String contentFile;
String contentJson;
String requestType;

ConfigurationReceiver::ConfigurationReceiver()
    : receivingPacket(false), expectedPacketSize(0), currentPacketSize(0),
      filesReceived(0), configurationsReceived(0), lastCommand(CMD_LIST_FILES),
      lastResponse(RESP_OK), pendingResponseData("")
{
}

bool ConfigurationReceiver::tryHandleChunkedTransfer(const uint8_t *data, size_t length,
                                                     uint8_t headerByte)
{
    if (length < 10)
    {
        return false;
    }

    if (data[0] != headerByte || data[1] != 0x00 || data[2] != 0xAD || data[3] != 0xDE)
    {
        return false;
    }

    uint16_t chunkNumber = (data[4] << 8) | data[5];
    uint16_t payloadSize = (data[8] << 8) | data[9];

    if (headerByte == 0x0D)
    {
        LOG_I("\n=== CHUNKED TRANSFER DETECTED ===");
    }
    else if (headerByte == 0x02)
    {
        LOG_I("\n=== CHUNKED TRANSFER DETECTED (TYPE 2) ===");
    }
    else
    {
        LOG_I("\n=== CHUNKED TRANSFER DETECTED (UNKNOWN TYPE) ===");
    }

    LOG_I("Chunk number: %d", chunkNumber);
    LOG_I("Payload size: %d bytes", payloadSize);

    if (length > 10)
    {
        String chunkPayload = "";
        for (size_t i = 10; i < length; ++i)
        {
            chunkPayload += static_cast<char>(data[i]);
        }

        LOG_I("--- CHUNK PAYLOAD ---");
        Serial.println(chunkPayload);
        LOG_I("--- END CHUNK ---");

        receivedData += chunkPayload;

        bool isLastChunk = false;
        size_t startIndex = length > 50 ? length - 50 : 0;
        for (size_t i = startIndex; i < length; ++i)
        {
            if (data[i] == 0x00)
            {
                isLastChunk = true;
                break;
            }
        }

        if (isLastChunk)
        {
            LOG_I("\n=== COMPLETE MESSAGE RECEIVED ===");
            LOG_I("Total payload length: %d characters", receivedData.length());

            parseCommand(receivedData);
            receivedData = "";
        }
    }

    return true;
}

void ConfigurationReceiver::processReceivedData(const uint8_t *data, size_t length)
{
    lastResponse = RESP_OK;
    LOG_I("=== CONFIGURATION DATA RECEIVED ===");
    LOG_I("Length: %d bytes", length);

    // // Display raw data as hex
    // Serial.print("Hex: ");
    // for (size_t i = 0; i < length; i++) {
    //     LOG_I("%02X ", data[i]);
    //     if ((i + 1) % 16 == 0) Serial.println();
    // }
    // Serial.println();
    //
    // // Als String interpretieren
    // String receivedString = "";
    // for (size_t i = 0; i < length; i++) {
    //     if (data[i] >= 32 && data[i] <= 126) { // Printable ASCII characters
    //         receivedString += (char)data[i];
    //     } else {
    //         receivedString += "[" + String(data[i], HEX) + "]";
    //     }
    // }
    // LOG_I("String interpretation:");
    // Serial.println(receivedString);

    // Check for chunked transfer protocol
    if (tryHandleChunkedTransfer(data, length, 0x0D))
    {
        // Already processed
    }
    else if (tryHandleChunkedTransfer(data, length, 0x02))
    {
        // Already processed
    }
    else
    {
        // Direct command (classic protocol)
        for (size_t i = 0; i < length; i++)
        {
            receivedData += (char)data[i];
        }
        LOG_I("\n=== PARSING COMMAND ===");
        parseCommand(receivedData);
        receivedData = "";
    }

    LOG_I("=====================================\n");
}

void ConfigurationReceiver::parseCommand(const String &data)
{
    pendingResponseData = "";

    if (data.length() == 0)
    {
        LOG_I("Empty data received");
        lastResponse = RESP_UNKNOWN_COMMAND;
        return;
    }

    // Check for direct commands in the combined string
    LOG_I("Total data length: %d characters", data.length());

    // PRIORITY 1: Check for JSON configuration (starts with '{' or contains 0x03)
    int jsonStart = data.indexOf('{');
    int configPos = data.indexOf((char)CMD_NEW_CONFIG);

    if (jsonStart >= 0 && (configPos < 0 || jsonStart < configPos))
    {
        LOG_I(">>> JSON CONFIGURATION DETECTED (config.json)");
        lastCommand = CMD_NEW_CONFIG; // Store command for response

        String jsonData = data.substring(jsonStart);
        // Clean JSON (remove null bytes)
        jsonData.replace("\0", "");

        logReceivedConfig(jsonData);
        configurationsReceived++;
        return;
    }

    // PRIORITY 2: Look for config pattern (0x03 + JSON)
    if (configPos >= 0)
    {
        LOG_I(">>> CONFIGURATION COMMAND FOUND (0x03)");
        lastCommand = CMD_NEW_CONFIG; // Store command for response

        // Extract starting at config position
        String configData = data.substring(configPos);
        handleConfigCommand(configData);
        configurationsReceived++;
        return;
    }

    // PRIORITY 3: Search for file-save pattern (0x02 + filename + content)
    int fileSavePos = data.indexOf((char)CMD_SAVE_FILE);
    if (fileSavePos >= 0)
    {
        LOG_I(">>> FILE SAVE COMMAND FOUND (0x02)");
        lastCommand = CMD_SAVE_FILE; // Store command for response

        // Extract starting from the file-save position
        String fileData = data.substring(fileSavePos);
        handleFileCommand(fileData);
        filesReceived++;
        return;
    }

    // PRIORITY 4: Detect Python code
    if (data.indexOf("import") >= 0 || data.indexOf("def ") >= 0 || data.indexOf("class ") >= 0)
    {
        LOG_I(">>> PYTHON CODE DETECTED");
        lastCommand = CMD_SAVE_FILE; // Treat as file save

        // Try to extract the file name (often at the start of the data)
        String filename = "detected_python_file.py";

        // Search for .py in the string
        int pyPos = data.indexOf(".py");
        if (pyPos >= 0)
        {
            // Search backward for the start of the file name
            int nameStart = pyPos - 20; // Max 20 characters for file name
            if (nameStart < 0)
                nameStart = 0;

            for (int i = pyPos - 1; i >= nameStart; i--)
            {
                char c = data[i];
                if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                    (c >= '0' && c <= '9') || c == '_')
                {
                    continue;
                }
                else
                {
                    filename = data.substring(i + 1, pyPos + 3);
                    break;
                }
            }
        }

        logReceivedFile(filename, data);
        filesReceived++;
        return;
    }

    // Fallback: try interpreting as the first command
    uint8_t command = (uint8_t)data[0];
    LOG_I("Command Byte: 0x%02X (%d)", command, command);

    // Store command for proper response
    lastCommand = (ConfigCommand)command;

    switch (command)
    {
    case CMD_LIST_FILES:
        LOG_I(">>> COMMAND: LIST FILES (LS)");
        LOG_I("App requested file list");
        pendingResponseData = buildFileListJson();
        lastResponse = RESP_OK;
        break;

    case CMD_GET_FILE:
        LOG_I(">>> COMMAND: GET FILE (FG)");
        LOG_I("App requested file content");
        if (data.length() > 1)
        {
            String filename = extractFilename(data, 100);
            filename.trim();
            LOG_I("Requested file: '%s'", filename.c_str());

            if (filename.length() == 0)
            {
                LOG_W("No valid file provided in request");
                lastResponse = RESP_FILE_NOT_FOUND;
                pendingResponseData = "";
            }
            else
            {
                String fileContent;
                if (loadFileContent(filename, fileContent))
                {
                    pendingResponseData = fileContent;
                    lastResponse = RESP_OK;
                }
                else
                {
                    LOG_W("File '%s' could not be read", filename.c_str());
                    pendingResponseData = "";
                    lastResponse = RESP_FILE_NOT_FOUND;
                }
            }
        }
        else
        {
            LOG_W("GET FILE command received without filename");
            pendingResponseData = "";
            lastResponse = RESP_FILE_NOT_FOUND;
        }
        break;

    case CMD_SAVE_FILE:
        LOG_I(">>> COMMAND: SAVE FILE (FS)");
        handleFileCommand(data);
        filesReceived++;
        break;

    case CMD_NEW_CONFIG:
        LOG_I(">>> COMMAND: NEW CONFIGURATION (C)");
        handleConfigCommand(data);
        configurationsReceived++;
        break;

    default:
        LOG_I(">>> UNKNOWN COMMAND: 0x%02X", command);
        LOG_I("Unknown data type");
        lastResponse = RESP_UNKNOWN_COMMAND;
        break;
    }
}

void ConfigurationReceiver::handleFileCommand(const String &data)
{
    if (data.length() < 102)
    { // 1 Byte Command + 100 Byte Filename + mindestens 1 Byte Content
        LOG_I("File command too short");
        lastResponse = RESP_UNKNOWN_COMMAND;
        return;
    }

    // Extract filename (bytes 1-100)
    String filename = extractFilename(data, 100);
    if (filename.length() == 0)
    {
        LOG_I("No valid filename found in command");
        lastResponse = RESP_UNKNOWN_COMMAND;
        return;
    }

    // File content (starting at byte 101)
    String fileContent = data.substring(101);

    logReceivedFile(filename, fileContent);
}

void ConfigurationReceiver::handleConfigCommand(const String &data)
{
    if (data.length() < 2)
    {
        LOG_I("Config command too short");
        lastResponse = RESP_UNKNOWN_COMMAND;
        return;
    }

    // JSON configuration (starting at byte 1)
    String configJson = data.substring(1);

    logReceivedConfig(configJson);
}

String ConfigurationReceiver::extractFilename(const String &data, int maxFilenameSize)
{
    String filename = "";

    // Extract the filename from bytes 1 through maxFilenameSize
    for (int i = 1; i <= maxFilenameSize && i < data.length(); i++)
    {
        char c = data[i];
        if (c == '\0')
            break; // Null terminator
        if (c >= 32 && c <= 126)
        { // Printable ASCII characters
            filename += c;
        }
    }

    return filename;
}

void ConfigurationReceiver::logReceivedFile(const String &filename, const String &content)
{
    LOG_I("\n┌─────────────────────────────────────────┐");
    LOG_I("│                FILE RECEIVED            │");
    LOG_I("├─────────────────────────────────────────┤");
    LOG_I("│ Filename: %-28s │", filename.c_str());
    LOG_I("│ Size:     %-28d │", content.length());
    LOG_I("└─────────────────────────────────────────┘");

    LOG_I("\n--- FILE CONTENT ---");
    if (content.length() > 35000)
    {
        Serial.println(content.substring(0, 35000));
        LOG_I("... (%d additional characters)", content.length() - 35000);
    }
    else
    {
        Serial.println(content);
    }
    LOG_I("--- END FILE CONTENT ---\n");

    contentFile = content;

    String targetPath = resolveFilePath(filename);
    if (targetPath.length() == 0)
    {
        LOG_E("Invalid file path – file will not be saved");
        lastResponse = RESP_UNKNOWN_COMMAND;
        return;
    }

    uint8_t cardType = SD.cardType();
#if defined(CARD_NONE)
    if (cardType == CARD_NONE)
#else
    if (cardType == 0)
#endif
    {
        LOG_E("No SD card available – saving aborted");
        lastResponse = RESP_FILE_NOT_FOUND;
        return;
    }

    int lastSlash = targetPath.lastIndexOf('/');
    String directory = "/";
    if (lastSlash > 0)
    {
        directory = targetPath.substring(0, lastSlash);
    }

    if (!ensureDirectoryExists(directory))
    {
        LOG_E("Target directory '%s' could not be created", directory.c_str());
        lastResponse = RESP_FILE_NOT_FOUND;
        return;
    }

    if (SD.exists(targetPath.c_str()))
    {
        if (!SD.remove(targetPath.c_str()))
        {
            LOG_E("File '%s' could not be removed", targetPath.c_str());
            lastResponse = RESP_FILE_NOT_FOUND;
            return;
        }
    }

    File file = SD.open(targetPath.c_str(), FILE_WRITE);
    if (!file)
    {
        LOG_E("File '%s' could not be opened for writing", targetPath.c_str());
        lastResponse = RESP_FILE_NOT_FOUND;
        return;
    }

    const size_t expectedLength = content.length();
    size_t writtenLength = expectedLength;
    if (expectedLength > 0)
    {
        writtenLength = file.write(reinterpret_cast<const uint8_t *>(content.c_str()), expectedLength);
        if (writtenLength != expectedLength)
        {
            LOG_E("File '%s' could not be written completely (%lu/%lu bytes)",
                  targetPath.c_str(), static_cast<unsigned long>(writtenLength),
                  static_cast<unsigned long>(expectedLength));
            file.close();
            lastResponse = RESP_FILE_NOT_FOUND;
            return;
        }
    }

    file.flush();
    file.close();

    LOG_I("💾 File saved to: %s (%lu bytes)", targetPath.c_str(),
          static_cast<unsigned long>(writtenLength));
    lastResponse = RESP_OK;

    // Count lines while config upload is running
    int lines = 1;
    for (int i = 0; i < content.length(); i++)
    {
        if (content[i] == '\n')
            lines++;
    }
    LOG_I("File '%s' has %d lines", filename.c_str(), lines);
    LOG_I("Preparing OK response for app...");
}

String ConfigurationReceiver::resolveFilePath(const String &filename)
{
    String trimmed = filename;
    trimmed.trim();
    if (trimmed.length() == 0)
    {
        return "";
    }

    String normalized = trimmed;
    normalized.replace("\\", "/");

    if (normalized.indexOf("..") >= 0)
    {
        LOG_E("Paths containing '..' are not allowed");
        return "";
    }

    if (!normalized.startsWith("/"))
    {
        normalized = "/" + normalized;
    }

    // Remove duplicate slashes
    while (normalized.indexOf("//") >= 0)
    {
        normalized.replace("//", "/");
    }

    // If only a filename without a directory was provided, store it in CONFIG_UPDATE_DIR (e.g., /config_update)
    int secondSlash = normalized.indexOf('/', 1);
    if (secondSlash < 0)
    {
        normalized = String(CONFIG_UPDATE_DIR) + normalized;
    }

    return normalized;
}

bool ConfigurationReceiver::ensureDirectoryExists(const String &directoryPath)
{
    if (directoryPath.length() == 0 || directoryPath == "/")
    {
        return true;
    }

    String normalized = directoryPath;
    normalized.trim();
    if (!normalized.startsWith("/"))
    {
        normalized = "/" + normalized;
    }

    while (normalized.indexOf("//") >= 0)
    {
        normalized.replace("//", "/");
    }

    String currentPath = "";
    int startIndex = 1;
    while (startIndex <= normalized.length())
    {
        int nextSlash = normalized.indexOf('/', startIndex);
        String segment;
        if (nextSlash == -1)
        {
            segment = normalized.substring(startIndex);
            startIndex = normalized.length() + 1;
        }
        else
        {
            segment = normalized.substring(startIndex, nextSlash);
            startIndex = nextSlash + 1;
        }

        if (segment.length() == 0)
        {
            continue;
        }

        currentPath += "/";
        currentPath += segment;

        if (!SD.exists(currentPath.c_str()))
        {
            LOG_I("📁 Creating directory: %s", currentPath.c_str());
            if (!SD.mkdir(currentPath.c_str()))
            {
                LOG_E("Directory '%s' could not be created", currentPath.c_str());
                return false;
            }
        }
    }

    return true;
}

String ConfigurationReceiver::buildFileListJson()
{
    std::vector<String> files;

    auto addFileIfUnique = [&files](const String &name)
    {
        if (name.length() == 0)
        {
            return;
        }

        for (const auto &existing : files)
        {
            if (existing.equalsIgnoreCase(name))
            {
                return;
            }
        }

        files.push_back(name);
    };

    auto collectFromDirectory = [&](const char *directoryPath)
    {
        if (!directoryPath || !SD.exists(directoryPath))
        {
            return;
        }

        File directory = SD.open(directoryPath);
        if (!directory)
        {
            return;
        }

        while (true)
        {
            File entry = directory.openNextFile();
            if (!entry)
            {
                break;
            }

            if (!entry.isDirectory())
            {
                String fileName = entry.name();
                int lastSlash = fileName.lastIndexOf('/');
                if (lastSlash >= 0)
                {
                    fileName = fileName.substring(lastSlash + 1);
                }

                String lowerName = fileName;
                lowerName.toLowerCase();
                if (lowerName.endsWith(".py") || lowerName.endsWith(".json"))
                {
                    addFileIfUnique(fileName);
                }
            }

            entry.close();
        }

        directory.close();
    };

    collectFromDirectory(CONFIG_ACTIVE_DIR);

    std::sort(files.begin(), files.end(), [](const String &a, const String &b)
              { return a.compareTo(b) < 0; });

    DynamicJsonDocument doc(256 + files.size() * 64);
    JsonArray list = doc.to<JsonArray>();
    for (const auto &file : files)
    {
        list.add(file);
    }

    String serialized;
    serializeJson(list, serialized);
    return serialized;
}

bool ConfigurationReceiver::loadFileContent(const String &filename, String &content)
{
    content = "";

    String normalized = filename;
    normalized.trim();
    if (normalized.length() == 0)
    {
        return false;
    }

    auto tryOpenFile = [&](const String &path)
    {
        if (path.length() == 0)
        {
            return false;
        }

        File file = SD.open(path.c_str(), FILE_READ);
        if (!file)
        {
            return false;
        }

        String data = "";
        while (file.available())
        {
            data += static_cast<char>(file.read());
        }
        file.close();

        LOG_I("📄 File '%s' read (%d bytes)", path.c_str(), static_cast<int>(data.length()));
        content = data;
        return true;
    };

    if (normalized.startsWith("/"))
    {
        if (tryOpenFile(normalized))
        {
            return true;
        }
    }
    else
    {
        const char *searchRoots[] = {CONFIG_ACTIVE_DIR, CONFIG_UPDATE_DIR, "/"};
        for (const char *root : searchRoots)
        {
            String path = String(root ? root : "");
            if (!path.endsWith("/"))
            {
                path += "/";
            }
            path += normalized;

            while (path.indexOf("//") >= 0)
            {
                path.replace("//", "/");
            }

            if (tryOpenFile(path))
            {
                return true;
            }
        }
    }

    return false;
}

void ConfigurationReceiver::logReceivedConfig(const String &configJson)
{
    LOG_I("\n┌─────────────────────────────────────────┐");
    LOG_I("│          CONFIGURATION RECEIVED        │");
    LOG_I("├─────────────────────────────────────────┤");
    LOG_I("│ Filename: %-28s │", "config.json");
    LOG_I("│ JSON size: %-26d │", configJson.length());
    LOG_I("└─────────────────────────────────────────┘");

    LOG_I("\n--- JSON CONFIGURATION ---");
    Serial.println(configJson);
    LOG_I("--- END JSON ---\n");

    contentJson = configJson;
    lastResponse = RESP_OK;

    // Try to parse JSON for better display
    DynamicJsonDocument doc(configJson.length() + 1024);
    DeserializationError error = deserializeJson(doc, configJson);

    if (error)
    {
        LOG_I("JSON parse error: %s", error.c_str());
        lastResponse = RESP_JSON_ERROR;
    }
    else
    {
        LOG_I("--- PARSED JSON (Pretty Print) ---");
        serializeJsonPretty(doc, Serial);
        LOG_I("\n--- END PARSED JSON ---");

        // Extract key information
        if (doc.containsKey("interfaceMcu"))
        {
            JsonObject ifaceMcu = doc["interfaceMcu"];
            if (ifaceMcu.containsKey("pythonFileNames"))
            {
                JsonArray files = ifaceMcu["pythonFileNames"];
                LOG_I("Expected Python files (%d):", files.size());
                for (JsonVariant file : files)
                {
                    LOG_I("  - %s", file.as<String>().c_str());
                }
            }
        }
    }

    LOG_I("Preparing OK response for app...!");
}

bool moveConfig(const char *srcPath, const char *dstPath)
{
    // Open source file
    File src = SD.open(srcPath, FILE_READ);
    if (!src)
    {
        return false; // Source not found
    }

    // If destination already exists, remove it
    if (SD.exists(dstPath))
    {
        SD.remove(dstPath);
    }

    // Create destination
    File dst = SD.open(dstPath, FILE_WRITE);
    if (!dst)
    {
        src.close();
        return false; // Destination could not be created
    }

    // Copy contents
    uint8_t buf[64];
    while (src.available())
    {
        int n = src.read(buf, sizeof(buf));
        if (n > 0)
            dst.write(buf, n);
    }

    src.close();
    dst.close();

    // Delete source file = move
    // SD.remove(srcPath);

    return true;
}

void ConfigurationReceiver::printStatistics()
{
    LOG_I("\n=== CONFIGURATION RECEIVER STATISTICS ===");
    LOG_I("Files received: %d", filesReceived);
    LOG_I("Configurations received: %d", configurationsReceived);
    LOG_I("==========================================\n");
}

bool isValidJsonOnSD(const char *path, size_t capacity = 2048)
{
    File f = SD.open(path, FILE_READ);
    if (!f)
    {
        return false; // File not found or not readable
    }

    DynamicJsonDocument doc(capacity);
    DeserializationError err = deserializeJson(doc, f);
    f.close();

    return !err; // true = valid, false = invalid
}

void getContentJson()
{
    SD.mkdir(CONFIG_UPDATE_DIR);
    SD.mkdir(CONFIG_ACTIVE_DIR);
    if (SD.exists(CONFIG_UPDATE_FILE) && !SD.remove(CONFIG_UPDATE_FILE))

    {
        LOG_I("Error while removing file %s!", CONFIG_UPDATE_FILE);
    }
    else
    {
        File file = SD.open(CONFIG_UPDATE_FILE, FILE_WRITE);
        if (file)
        {
            if (!file.seek(0))
            {
                LOG_I("Error positioning file %s!", CONFIG_UPDATE_FILE);
            }
            else
            {
                file.print(contentJson);
                file.flush();
                LOG_I("config saved successfully!");

                if (isValidJsonOnSD(CONFIG_UPDATE_FILE))
                {
                    Serial.println("JSON is valid ✅");
                    if (moveConfig(CONFIG_UPDATE_FILE, CONFIG_FILE))
                    {
                        Serial.println("Config moved successfully ✅");
                        LOG_I("Config update start ");
                        if (isValidJsonOnSD(CALIB_UPDATE_FILE))
                        {
                            moveConfig(CALIB_UPDATE_FILE, CALIB_FILE);
                        }
                        loadConfiguration();
                        synchronizeTime();
                        verifySystemHealth();
                        initializeConfiguredSensors();
                        measurementStartStop();
                        sensorPrepDurationTime();
                        LOG_I("Config update done ");
                        //! esp_deep_sleep(1000000ULL); //1sec
                    }
                    else
                    {
                        Serial.println("Error while moving ❌");
                    }
                }
                else
                {
                    Serial.println("JSON invalid ❌");
                }
            }
            file.close();
        }
        else

        {
            LOG_I("Error while opening file %s!", CONFIG_UPDATE_FILE);
        }
    }
}

void ConfigurationReceiver::reset()
{
    receivedData = "";
    receivingPacket = false;
    expectedPacketSize = 0;
    currentPacketSize = 0;
    filesReceived = 0;
    configurationsReceived = 0;
    lastCommand = CMD_LIST_FILES;
    lastResponse = RESP_OK;
    pendingResponseData = "";
    LOG_I("Configuration Receiver reset");
}

// DEPRECATED: Old response function (for compatibility)
String ConfigurationReceiver::generateResponse(ConfigResponse response, const String &data)
{
    String responseStr = "";
    responseStr += (char)response; // Response code as first byte
    responseStr += data;           // Optional additional data

    LOG_I("OLD response generated: Code=0x%02X, Data='%s'", response, data.c_str());
    return responseStr;
}

String ConfigurationReceiver::generateProperResponse(ConfigResponse response, const String &data)
{
    String responseStr = "";

    lastResponse = response;

    const String &payload = data.length() > 0 ? data : pendingResponseData;

    bool simpleResponse = (lastCommand == CMD_LIST_FILES || lastCommand == CMD_GET_FILE);

    // NEW (ASCII for FS/C, binary for LS/FG):
    if (simpleResponse)
    {
        // LS / FG: unchanged (1 byte + payload)
        responseStr += (char)response;
        if (payload.length() > 0)
        {
            responseStr += payload;
        }
    }
    else
    {
        // FS / C: ASCII "20" or "30" etc.
        responseStr += String((int)lastCommand); // "2" or "3"
        responseStr += String((int)response);    // "0" (= OK) etc.
        if (payload.length() > 0)
        {
            responseStr += payload;
        }
    }

    LOG_I("=====================================");
    Serial.println();
    LOG_I("=== PROPER RESPONSE GENERATED ===");
    LOG_I("Request Type: 0x%02X (%s)", lastCommand,
          lastCommand == CMD_SAVE_FILE ? "FS" : lastCommand == CMD_NEW_CONFIG ? "C"
                                            : lastCommand == CMD_GET_FILE     ? "FG"
                                            : lastCommand == CMD_LIST_FILES   ? "LS"
                                                                              : "UNKNOWN");
    LOG_I("Response Code: 0x%02X (%s)", response,
          response == RESP_OK ? "OK" : response == RESP_JSON_ERROR   ? "JSON_ERROR"
                                   : response == RESP_INVALID_CONFIG ? "INVALID_CONFIG"
                                   : response == RESP_FILE_NOT_FOUND ? "FILE_NOT_FOUND"
                                                                     : "UNKNOWN");
    LOG_I("Additional Data: '%s'", data.c_str());
    if (simpleResponse)
    {
        LOG_I("Complete Response: [0x%02X] + data", response);
    }
    else
    {
        LOG_I("Complete Response: [0x%02X, 0x%02X] + data", lastCommand, response);
    }
    LOG_I("Response Length: %d bytes", responseStr.length());
    LOG_I("=====================================================");

    requestType = lastCommand;

    pendingResponseData = "";

    return responseStr;
}