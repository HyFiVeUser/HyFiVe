#include "measurement_storage.h"

#include <SD.h>
#include <SPI.h>

#include "init.h"
#include "logging.h"
#include "sensors.h"
#include "sd_csv_backend.h"
#include "led.h"

bool s_baseReady = false;
String s_measurementFilePath;
File s_currentFile;
const char *const INDEX_FILE_NAME = "index.csv";
const char *const MEASUREMENTS_FILE_NAME = "datM.csv";
constexpr size_t MEASUREMENT_RECORD_BYTES = 96;

struct IndexSnapshot
{
    uint32_t measuredCount = 0;
    uint32_t transferredCount = 0;
};

void ensureIndexFileInitialized();

static void writeFixedLengthCsvRecord(File &file, const String &record)
{
    String padded = record;
    const size_t targetLength = MEASUREMENT_RECORD_BYTES - 1; // keep last byte for \n

    if (padded.length() > targetLength)
    {
        LOG_W("Measurement line exceeds fixed length (%u > %u): %s", static_cast<unsigned>(padded.length()), static_cast<unsigned>(targetLength), record.c_str());
    }
    else
    {
        padded.reserve(MEASUREMENT_RECORD_BYTES);
        while (padded.length() < targetLength)
        {
            padded += ' ';
        }
    }

    padded += '\n';
    file.print(padded);
}

void closeCurrentFile()
{
    if (s_currentFile)
    {
        s_currentFile.close();
        s_currentFile = File();
    }
}

String buildIndexFilePath()
{
    return String(MEASUREMENTS_ROOT_DIR) + "/" + INDEX_FILE_NAME;
}

String buildMeasurementFilePath()
{
    return String(MEASUREMENTS_ROOT_DIR) + "/" + MEASUREMENTS_FILE_NAME;
}

IndexSnapshot readIndexSnapshot()
{
    IndexSnapshot snapshot;

    String indexPath = buildIndexFilePath();
    if (!SD.exists(indexPath.c_str()))
    {
        ensureIndexFileInitialized();
        return snapshot;
    }

    File file = SD.open(indexPath.c_str(), FILE_READ);
    if (!file)
    {
        return snapshot;
    }

    file.readStringUntil('\n');

    String values = file.readStringUntil('\n');
    file.close();
    values.trim();
    if (!values.length())
    {
        return snapshot;
    }

    int fieldIndex = 0;
    int start = 0;
    while (start <= values.length())
    {
        int comma = values.indexOf(',', start);
        String part;
        if (comma < 0)
        {
            part = values.substring(start);
            start = values.length() + 1;
        }
        else
        {
            part = values.substring(start, comma);
            start = comma + 1;
        }
        part.trim();

        switch (fieldIndex)
        {
        case 0:
            snapshot.measuredCount = static_cast<uint32_t>(part.toInt());
            break;
        case 1:
            snapshot.transferredCount = static_cast<uint32_t>(part.toInt());
            break;
        default:
            break;
        }

        ++fieldIndex;
    }

    return snapshot;
}

static uint32_t countMeasurementFileLines()
{
    String measurementPath = buildMeasurementFilePath();
    if (!SD.exists(measurementPath.c_str()))
    {
        return 0;
    }

    File file = SD.open(measurementPath.c_str(), FILE_READ);
    if (!file)
    {
        LOG_E("Could not read measurement file: %s", measurementPath.c_str());
        return 0;
    }

    String header = file.readStringUntil('\n');
    size_t headerBytes = header.length();
    if (file.available())
    {
        headerBytes += 1;
    }

    const uint32_t totalBytes = static_cast<uint32_t>(file.size());
    if (totalBytes <= headerBytes)
    {
        file.close();
        return 0;
    }

    const uint32_t dataBytes = totalBytes - static_cast<uint32_t>(headerBytes);
    file.close();

    return dataBytes / MEASUREMENT_RECORD_BYTES;
}

void writeIndexSnapshot(const IndexSnapshot &snapshot)
{
    String indexPath = buildIndexFilePath();
    //! SD.remove(indexPath.c_str());

    File file = SD.open(indexPath.c_str(), FILE_WRITE);
    if (!file)
    {
        LOG_E("Could not write index file: %s", indexPath.c_str());
        return;
    }

    file.println("measured,transmitted_measured");
    file.print(snapshot.measuredCount);
    file.print(',');
    file.print(snapshot.transferredCount);
    file.println();
    file.flush();
    file.close();
}

void ensureIndexFileInitialized()
{
    String indexPath = buildIndexFilePath();
    for (int i = 0; i < 10; i++)
    {
        if (SD.exists(indexPath.c_str()))
        {
            return;
        }
        delay(100);
    }

    IndexSnapshot snapshot;
    writeIndexSnapshot(snapshot);

    LOG_I("🗂️ Index file initialized: %s", indexPath.c_str());
}

bool ensureBaseDirectory()
{
    if (s_baseReady)
    {
        return true;
    }

    if (!SD.exists(MEASUREMENTS_ROOT_DIR))
    {
        if (!SD.mkdir(MEASUREMENTS_ROOT_DIR))
        {
            LOG_E("Could not create measurements directory: %s", MEASUREMENTS_ROOT_DIR);
            return false;
        }
    }

    s_baseReady = SD.exists(MEASUREMENTS_ROOT_DIR);
    if (s_baseReady)
    {
        LOG_I("📁 Measurements directory ready: %s", MEASUREMENTS_ROOT_DIR);
        ensureIndexFileInitialized();
    }
    return s_baseReady;
}

String buildTimestampString(const DateTime &now)
{
    char buffer[25];
    snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
             now.year(), now.month(), now.day(),
             now.hour(), now.minute(), now.second());
    return String(buffer);
}

bool openMeasurementFile()
{
    if (!ensureBaseDirectory())
    {
        return false;
    }

    if (!s_measurementFilePath.length())
    {
        s_measurementFilePath = buildMeasurementFilePath();
    }

    bool fileExists = SD.exists(s_measurementFilePath.c_str());

    closeCurrentFile();

    s_currentFile = SD.open(s_measurementFilePath.c_str(), FILE_APPEND);
    if (!s_currentFile)
    {
        LOG_E("Measurement file could not be opened: %s", s_measurementFilePath.c_str());
        return false;
    }

    if (!fileExists)
    {
        s_currentFile.println("timestamp,id,lat,lng,nvalue,svalue");
        s_currentFile.flush();
        LOG_I("🆕 Measurement file created: %s", s_measurementFilePath.c_str());
    }
    else
    {
        LOG_I("📄 Measurements will continue in: %s", s_measurementFilePath.c_str());
    }

    sd_csv_set_path(s_measurementFilePath);
    return true;
}

bool validateMeasurementIndex()
{
    IndexSnapshot snapshot = readIndexSnapshot();
    uint32_t measuredCount = 0;
    uint32_t transferredCount = 0;
    getMeasurementIndexCounts(measuredCount, transferredCount);

    bool ok = true;
    if (measuredCount < transferredCount)
    {
        LOG_E("Error: measured (%u) is smaller than transmitted_measured (%u).",
              static_cast<unsigned>(measuredCount),
              static_cast<unsigned>(transferredCount));
        snapshot.transferredCount = 0;
        writeIndexSnapshot(snapshot);
        ok = false;
    }
    else
    {
        Serial.println("measuredCount and transferredCount: ");
        Serial.print("measuredCount: ");
        Serial.println(measuredCount);
        Serial.print("transferredCount: ");
        Serial.println(transferredCount);
    }

    delay(100);

    uint32_t measurementLines = countMeasurementFileLines();
    if (measurementLines != measuredCount)
    {
        LOG_E("Error: datM.csv lines (%u) do not match measured (%u).",
              static_cast<unsigned>(measurementLines),
              static_cast<unsigned>(measuredCount));
        snapshot.measuredCount = measurementLines;
        snapshot.transferredCount = 0;
        writeIndexSnapshot(snapshot);
        ok = false;
    }
    else
    {
        Serial.println("datM.csv Lines: ");
        Serial.print("measurementLines: ");
        Serial.println(measurementLines);
        Serial.print("measuredCount: ");
        Serial.println(measuredCount);
    }

    return ok;
}

bool ensureCurrentFile(const DateTime &now)
{
    (void)now;
    if (s_currentFile)
    {
        return true;
    }
    return openMeasurementFile();
}

void finalizeCurrentFileIfNeeded() {}

void initMeasurementStorage()
{
    closeCurrentFile();
    s_baseReady = false;
    s_measurementFilePath = buildMeasurementFilePath();

    if (!openMeasurementFile())
    {
        LOG_W("Measurement file could not be initialized");
    }
}

void notifyMeasurementPaused()
{
    if (s_currentFile)
    {
        s_currentFile.flush();
    }
    closeCurrentFile();
}

void storeSensorMeasurements()
{
    ledSignalMeasurement();

    if (activeSensorCount == 0)
    {
        return;
    }

    DateTime now = rtc.now();
    if (!ensureCurrentFile(now))
    {
        return;
    }

    if (!s_currentFile)
    {
        LOG_E("Measurement file not writable: %s", s_measurementFilePath.c_str());
        return;
    }

    String timestamp = buildTimestampString(now);
    uint16_t newEntries = 0;

    for (uint8_t i = 0; i < activeSensorCount; ++i)
    {
        const auto &sensor = activeSensors[i];
        if (!sensor.active || !sensor.valueValid)
        {
            continue;
        }

        String line;
        line.reserve(MEASUREMENT_RECORD_BYTES);
        line += timestamp;
        line += ',';
        line += sensor.id;
        line += ",,,";
        line += String(sensor.currentValue, 4);
        line += ',';

        writeFixedLengthCsvRecord(s_currentFile, line);
        LOG_I("storeSensorMeasurements %s", line.c_str());
        ++newEntries;
    }

    s_currentFile.flush();
    if (newEntries > 0)
    {
        s_currentFile.flush();
        updateMeasurementIndex(static_cast<int32_t>(newEntries), 0);
    }
    finalizeCurrentFileIfNeeded();
}

void updateMeasurementIndex(int32_t newDelta, int32_t transferredDelta)
{
    //! if (!ensureBaseDirectory())
    //! {
    //!     return;
    //! }

    IndexSnapshot snapshot = readIndexSnapshot();

    if (snapshot.measuredCount == 0)
    {
        for (int i = 0; i < 5; i++)
        {
            Serial.println("snapshot.measuredCount");
            Serial.println(snapshot.measuredCount);
            Serial.println("snapshot.transferredCount");
            Serial.println(snapshot.transferredCount);

            snapshot = readIndexSnapshot();

            if (!snapshot.measuredCount == 0 || !snapshot.transferredCount == 0)
            {
                Serial.println("X snapshot.measuredCount");
                Serial.println(snapshot.measuredCount);
                Serial.println("X snapshot.transferredCount");
                Serial.println(snapshot.transferredCount);
                break;
            }
            delay(100);
        }
    }

    int64_t newTotal = static_cast<int64_t>(snapshot.measuredCount) + static_cast<int64_t>(newDelta);
    int64_t transferredTotal = static_cast<int64_t>(snapshot.transferredCount) + static_cast<int64_t>(transferredDelta);

    if (newTotal < 0)
    {
        newTotal = 0;
    }
    if (transferredTotal < 0)
    {
        transferredTotal = 0;
    }

    snapshot.measuredCount = static_cast<uint32_t>(newTotal);
    snapshot.transferredCount = static_cast<uint32_t>(transferredTotal);

    if (!snapshot.measuredCount == 0)
    {
        writeIndexSnapshot(snapshot);
    }
}

bool getMeasurementIndexCounts(uint32_t &measuredCount, uint32_t &transferredCount)
{
    measuredCount = 0;
    transferredCount = 0;

    //! if (!ensureBaseDirectory())
    //! {
    //!     return false;
    //! }

    IndexSnapshot snapshot = readIndexSnapshot();
    measuredCount = snapshot.measuredCount;
    transferredCount = snapshot.transferredCount;

    if (measuredCount == 0)
    {
        for (int i = 0; i < 5; i++)
        {
            Serial.println("measuredCount");
            Serial.println(measuredCount);
            Serial.println("transferredCount");
            Serial.println(transferredCount);

            snapshot = readIndexSnapshot();
            measuredCount = snapshot.measuredCount;
            transferredCount = snapshot.transferredCount;

            if (!measuredCount == 0 || !transferredCount == 0)
            {
                Serial.println("X measuredCount");
                Serial.println(measuredCount);
                Serial.println("X transferredCount");
                Serial.println(transferredCount);
                break;
            }
            delay(100);
        }
    }

    return true;
}