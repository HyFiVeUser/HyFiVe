/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Sample casting and analysis functions
 */

#include <SD.h>
#include <vector>

#include "DebuggingSDLog.h"
#include "SystemVariables.h"
#include "loggerConfig.h"
#include "sample_cast.h"

/**
 * @brief Keeps the latest sample cast lines in the file.
 */
void keepLatestSampleCastLines()
{
  const char *filePath = "/measurements/sample_cast.txt";
  int maxLines         = sampleCastIntervals + 10;
  int linesToKeep      = sampleCastIntervals + 5;

  File file = SD.open(filePath, FILE_READ);
  if (!file)
  {
    Serial.println("Error opening the file.");
    return;
  }

  // Count the lines
  int lineCount               = 0;
  long fileSize               = file.size();
  long positions[linesToKeep] = {0};

  for (long i = fileSize - 1; i >= 0; i--)
  {
    file.seek(i);
    if (file.read() == '\n' || i == 0)
    {
      lineCount++;
      if (lineCount <= linesToKeep)
      {
        positions[linesToKeep - lineCount] = (i == 0 && lineCount > 1) ? 0 : i + 1;
      }
    }
  }

  // If sampleCastIntervals + 1 or less lines, nothing to do
  if (lineCount <= maxLines)
  {
    file.close();
    return;
  }

  // Save the last lines
  String lastLines = "";
  file.seek(positions[0]);
  while (file.available())
  {
    lastLines += (char)file.read();
  }
  file.close();

  file = SD.open(filePath, FILE_WRITE);
  if (!file)
  {
    Log(LogCategorySDCard, LogLevelERROR, "sample_cast.txt, Error opening the file.");
    return;
  }

  // Write the last lines back into the file
  file.print(lastLines);
  file.close();

  Log(LogCategorySDCard, LogLevelDEBUG, "File has been updated. Only the last lines were kept.");
}

/**
 * @brief Performs sample cast analysis.
 * @return bool True if cast average speed is above threshold, false otherwise.
 */
bool performSampleCast()
{
  if (configRTC.sample_cast_enable == 0)
  {
    return true;
  }

  const char *filePath = "/measurements/sample_cast.txt";

  File file = SD.open(filePath, FILE_READ);
  if (!file)
  {
    Serial.println("Error opening the file.");
    Log(LogCategoryGeneral, LogLevelERROR, "Error opening the file. SampleCast");
    return false;
  }

  // Log entries
  std::vector<LogEntry> entries;
  while (file.available())
  {
    String line    = file.readStringUntil('\n');
    int commaIndex = line.indexOf(',');
    if (commaIndex != -1)
    {
      // Extracts the timestamp and print value from the line
      long timestamp = line.substring(0, commaIndex).toInt();
      int pressure   = line.substring(commaIndex + 1).toInt(); // Skip comma and space
      entries.push_back({String(timestamp), pressure});
    }
  }
  file.close();
  keepLatestSampleCastLines();

  // Check whether at least sampleCastIntervals + 1 entries are present
  if (entries.size() < sampleCastIntervals + 1)
  {
    Serial.println("Not enough data available for the calculation.");
    return true;
  }

  // Calculate the average speed of the pressure change over the last sampleCastIntervals lines
  long startTime  = entries[entries.size() - sampleCastIntervals - 1].time.toInt();
  long endTime    = entries[entries.size() - 1].time.toInt();
  double timeDiff = abs(endTime - startTime);

  int startPressure     = entries[entries.size() - sampleCastIntervals - 1].pressure;
  int endPressure       = entries[entries.size() - 1].pressure;
  double pressureChange = abs(endPressure - startPressure);

  double castAverageSpeed = 0;
  if (timeDiff > 0)
  {
    castAverageSpeed = pressureChange / timeDiff;
  }

  // Output of the formula with the actual values
  String formula = "castAverageSpeed = abs((" + String(endPressure) + " - " + String(startPressure) + ") / (" + String(endTime) + " - " + String(startTime) + "))";
  Log(LogCategoryGeneral, LogLevelDEBUG, "Formel: ", formula);
  Log(LogCategorySensors, LogLevelDEBUG, "Average speed of the last sampleCastIntervals rows: ", String(castAverageSpeed, 2), " Units/s");

  if (castAverageSpeed > configRTC.cast_det_sensor_threshold)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * @brief Creates or clears the sample file.
 */
void createOrClearSampleFile()
{
  File file = SD.open("/measurements/sample_cast.txt", FILE_WRITE);
  if (!file)
  {
    Serial.println("Error when creating the file.");
  }
  file.close();
  Serial.println("File 'sample_cast.txt' created.");
}