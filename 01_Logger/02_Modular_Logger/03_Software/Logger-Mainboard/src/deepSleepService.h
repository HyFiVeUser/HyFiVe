#ifndef DEEP_SLEEP_SERVICE_H
#define DEEP_SLEEP_SERVICE_H

#include <Arduino.h>

// Checks the RTC_ALARM_PIN for a wake signal.
// If the pin is not pulled low, deep sleep is started immediately.
// If the signal stays active for the entire confirmation period, the function returns true.
void confirmRtcAlarmWake();

// Monitors RTC_ALARM_PIN in the loop and enters deep sleep after 5 seconds of LOW.
void monitorRtcAlarmForSleep();

void esp32Error();

void initGpio13Watch();

void setGPIO_NUM_13();

void ledBatteryPercent(float percent);

void enterDeepSleep();

#endif // DEEP_SLEEP_SERVICE_H