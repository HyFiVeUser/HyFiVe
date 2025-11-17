#pragma once
#include <Arduino.h>
#include <SD.h>
#include "logging.h"

// Categories and levels inspired by HYFiVE – freely extendable
enum class LogCategory { General, BMS, Sensors, Configuration };
enum class LogLevel    { DEBUG, INFO, WARN, ERROR };

// Optional: make SD logging controllable
static bool g_logSdReady = false;
inline void setLogSdReady(bool ready) { g_logSdReady = ready; }

// Internal helper: concatenate everything into a single string
inline void _appendAll(String& out) {}
template<typename T, typename... Rest>
inline void _appendAll(String& out, const T& first, Rest... rest) {
  out += String(first);
  _appendAll(out, rest...);
}

// Map to OSBK log plus optional SD file (/log/log.txt)
template<typename... Args>
inline void Log(LogCategory cat, LogLevel lvl, Args... args) {
  // Build header: [BMS][INFO] ...
  const char* catStr = (cat == LogCategory::BMS) ? "BMS" :
                       (cat == LogCategory::Sensors) ? "SENS" :
                       (cat == LogCategory::Configuration) ? "CONF" : "GEN";
  const char* lvlStr = (lvl == LogLevel::DEBUG) ? "DEBUG" :
                       (lvl == LogLevel::INFO ) ? "INFO " :
                       (lvl == LogLevel::WARN ) ? "WARN " : "ERROR";

  String msg; _appendAll(msg, args...);

  // Output using existing macros
  if (lvl == LogLevel::ERROR) {
    LOG_E("[%s] %s", catStr, msg.c_str());
  } else if (lvl == LogLevel::WARN) {
    LOG_W("[%s] %s", catStr, msg.c_str());
  } else {
    LOG_I("[%s] %s", catStr, msg.c_str());
  }

  // Optionally also log to SD once the SD card is initialized
  if (g_logSdReady) {
    if (!SD.exists("/log")) SD.mkdir("/log");
    File f = SD.open("/log/log.txt", FILE_APPEND);
    if (f) {
      f.printf("[%s][%s] %s\n", catStr, lvlStr, msg.c_str());
      f.close();
    }
  }
}
