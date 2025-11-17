#pragma once

#include <Arduino.h>

#define LOG_I(fmt, ...) Serial.printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define LOG_W(fmt, ...) Serial.printf("[WARN] " fmt "\n", ##__VA_ARGS__)
#define LOG_E(fmt, ...) Serial.printf("[ERR ] " fmt "\n", ##__VA_ARGS__)

