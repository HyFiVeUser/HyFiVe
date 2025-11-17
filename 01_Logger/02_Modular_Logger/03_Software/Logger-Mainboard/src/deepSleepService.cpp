#include "deepSleepService.h"

#include "init.h"
#include "logging.h"
#include "ble_config.h"
#include "led.h"
#include <esp_sleep.h>
#include "sensors.h"
#include "driver/rtc_io.h"

RTC_DATA_ATTR bool isESPWakeUp = 0;

#define ALARM_PIN GPIO_NUM_13

volatile bool g_gpio13WentLow = false;
static bool g_rtcAlarmLogged = false;

bool logTriggeredRtcAlarm()
{
    const bool alarm1Fired = rtc.alarmFired(1);
    const bool alarm2Fired = rtc.alarmFired(2);

    if (alarm1Fired && alarm2Fired)
    {
        LOG_W("RTC alarms triggered: startEpoch & stopEpoch");
    }
    else if (alarm1Fired)
    {
        LOG_I("RTC alarm triggered: startEpoch (Alarm1)");
        rtc.clearAlarm(1);
        rtc.clearAlarm(2);
        return false;
    }
    else if (alarm2Fired)
    {
        LOG_I("RTC alarm triggered: stopEpoch (Alarm2)");
        return true;
    }
    else
    {
        LOG_W("RTC alarm triggered, but no alarm flags set");
    }
}

void IRAM_ATTR onGpio13Falling()
{
    g_gpio13WentLow = true; // observed LOW once -> remember it
}

// Call once in setup()
void initGpio13Watch()
{
    pinMode(ALARM_PIN, INPUT_PULLUP); // DS3231 INT is open-drain (active LOW)
    attachInterrupt(digitalPinToInterrupt(ALARM_PIN), onGpio13Falling, FALLING);

    // If the pin is already LOW at startup, mark it as seen immediately:
    if (digitalRead(ALARM_PIN) == LOW)
        g_gpio13WentLow = true;
}

// Usable across the codebase
bool gpio13HasGoneLow(bool clearFlag = true)
{
    bool hit = g_gpio13WentLow;
    if (clearFlag)
        g_gpio13WentLow = false; // Optionally reset flag
    return hit;
}

void initGpio13Watch();

constexpr uint32_t kConfirmationDurationMs = 2100; //! so the LED still lights on the third blink
constexpr uint32_t kPollIntervalMs = 10;
constexpr uint32_t kLedToggleIntervalMs = 500;
constexpr uint32_t kSleepRequestDurationMs = 4000;
constexpr uint8_t kStatusLedPin = 42;

void ensureStatusLedOff()
{
    pinMode(kStatusLedPin, OUTPUT);
    digitalWrite(kStatusLedPin, LOW);
}

void enterDeepSleep()
{
    // ledResetAllStates();
    ensureStatusLedOff();
    delay(500);
    LOG_I("ESP32 -> deep sleep");
    esp_deep_sleep_start();
}

void confirmRtcAlarmWake()
{
    if (isESPWakeUp == 1)
    {
        esp_sleep_enable_ext0_wakeup(RTC_ALARM_PIN, 0); // if the signal is pulled to 0V, the ESP32-S3 wakes up

        pinMode(RTC_ALARM_PIN, INPUT_PULLUP);

        if (digitalRead(RTC_ALARM_PIN) != LOW)
        {
            enterDeepSleep();
        }

        pinMode(kStatusLedPin, OUTPUT);
        digitalWrite(kStatusLedPin, HIGH);

        const uint32_t start = millis();
        uint32_t lastToggle = start;
        bool ledOn = true;
        while ((millis() - start) < kConfirmationDurationMs)
        {
            if (digitalRead(RTC_ALARM_PIN) != LOW)
            {
                enterDeepSleep();
            }

            const uint32_t now = millis();
            if ((now - lastToggle) >= kLedToggleIntervalMs)
            {
                ledOn = !ledOn;
                digitalWrite(kStatusLedPin, ledOn ? HIGH : LOW);
                lastToggle = now;
            }
            delay(kPollIntervalMs);
        }

        ensureStatusLedOff();
        LOG_I("Wake signal confirmed (>=2.1s)");
        isESPWakeUp = 0;
    }
}
bool loadAktivPressIO17 = 0;
void monitorRtcAlarmForSleep()
{
    if (gpio13HasGoneLow())
    {
        if (logTriggeredRtcAlarm())
        {
            g_rtcAlarmLogged = false;

            LOG_I("ESP go DeepSleep gpio13HasGoneLow");
            isESPWakeUp = 1;
            // 1) Release DS3231 INT; otherwise it wakes up immediately again
            rtc.clearAlarm(1);
            rtc.clearAlarm(2);
            disable5V();
            disable12V();
            for (int i = 0; i < 32; i++)
            {
                sleepSensor(i);
            }
            enterDeepSleep();
        }
    }

    if (loadAktivPressIO17 == 1)
    {
        ledResetAllStates();
        Serial.println("ledResetAllStates");
        loadAktivPressIO17 = 0;
        initStatusLed();
    }

    if (digitalRead(RTC_ALARM_PIN) != LOW)
    {
        return;
    }

    pinMode(kStatusLedPin, OUTPUT);
    digitalWrite(kStatusLedPin, HIGH);

    const uint32_t start = millis();
    uint32_t lastToggle = start;
    bool ledOn = true;

    while (digitalRead(RTC_ALARM_PIN) == LOW)
    {
        const uint32_t now = millis();

        if ((now - lastToggle) >= kLedToggleIntervalMs)
        {
            ledOn = !ledOn;
            digitalWrite(kStatusLedPin, ledOn ? HIGH : LOW);
            lastToggle = now;
        }

        if ((now - start) >= kSleepRequestDurationMs)
        {
            isESPWakeUp = 1;
            delay(5000);
            LOG_I("ESP go DeepSleep");
            disable5V();
            disable12V();
            for (int i = 0; i < 32; i++)
            {
                sleepSensor(i);
            }
            enterDeepSleep();
        }

        delay(kPollIntervalMs);
    }

    ensureStatusLedOff();
    loadAktivPressIO17 = 1;
}

void esp32Error()
{
    pinMode(kStatusLedPin, OUTPUT);
    digitalWrite(kStatusLedPin, HIGH);

    const uint32_t start = millis();
    uint32_t lastToggle = start;
    bool ledOn = true;

    while (1)
    {
        const uint32_t now = millis();
        if ((now - lastToggle) >= 50)
        {
            ledOn = !ledOn;
            digitalWrite(kStatusLedPin, ledOn ? HIGH : LOW);
            lastToggle = now;
        }

        if ((now - start) >= 10000)
        {
            disable5V();
            disable12V();
            for (int i = 0; i < 32; i++)
            {
                sleepSensor(i);
            }
            enterDeepSleep();
        }

        delay(kPollIntervalMs);
    }
}

void ledBatteryPercent(float percent)
{
    pinMode(kStatusLedPin, OUTPUT);

    if (percent < 0.0f)
    {
        percent = 0.0f;
    }
    else if (percent > 100.0f)
    {
        percent = 100.0f;
    }

    uint32_t onDurationMs = 500;
    uint32_t offDurationMs = 500;
    uint8_t pulseCount = 0;

    if (percent >= 100.0f)
    {
        pulseCount = 1;
        onDurationMs = 5000;
        offDurationMs = 0;
    }
    else if (percent < 10.0f)
    {
        pulseCount = 1;
        onDurationMs = 100;
        offDurationMs = 100;
    }
    else
    {
        pulseCount = static_cast<uint8_t>(percent) / 10;
    }

    digitalWrite(kStatusLedPin, LOW);
    delay(1000);

    for (uint8_t pulse = 0; pulse < pulseCount; ++pulse)
    {
        digitalWrite(kStatusLedPin, HIGH);
        delay(onDurationMs);

        digitalWrite(kStatusLedPin, LOW);

        if (offDurationMs > 0)
        {
            delay(offDurationMs);
        }
    }

    digitalWrite(kStatusLedPin, LOW);
    delay(1000);
}

void setGPIO_NUM_13()
{
    uint64_t mask = (1ULL << GPIO_NUM_13);
    esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_LOW);

    // GPIO13: external pull-up present -> disable internal pulls
    pinMode(GPIO_NUM_13, INPUT); // Set direction
    rtc_gpio_init(GPIO_NUM_13);
    rtc_gpio_set_direction(GPIO_NUM_13, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pullup_dis(GPIO_NUM_13);
    rtc_gpio_pulldown_dis(GPIO_NUM_13);
}