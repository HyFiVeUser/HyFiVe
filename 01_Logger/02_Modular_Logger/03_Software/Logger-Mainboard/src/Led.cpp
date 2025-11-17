#include "led.h"

#include <Arduino.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// -----------------------------------------------------------------------------
// Introduction
// -----------------------------------------------------------------------------
// The status LED is driven in its own FreeRTOS task so it can blink
// independently of the rest of the program without worrying about precise
// delays. Instead of a FreeRTOS queue we use a simple boolean flag that only
// records whether a new command arrived. That keeps the implementation
// understandable and is more than sufficient for our needs.
//
// Every step is documented in detail. We intentionally avoid advanced language
// features such as "enum class" and stick to straightforward constants and
// if-statements.

namespace
{
    // Status LED pin number on the board.
    constexpr int STATUS_LED_PIN = 42;

    // LEDC configuration for PWM control of the status LED.
    constexpr uint8_t LEDC_CHANNEL = 0;
    constexpr uint32_t LEDC_FREQUENCY = 5000;
    constexpr uint8_t LEDC_RESOLUTION = 8;
    constexpr uint16_t LED_MAX_DUTY = (1 << LEDC_RESOLUTION) - 1;

    struct BootPatternTiming
    {
        uint8_t blinkCount;
        uint32_t onMs;
        uint32_t offMs;
    };

    struct ErrorPatternTiming
    {
        uint32_t totalDurationMs;
        uint32_t stepDurationMs;
    };

    struct MeasurementPatternTiming
    {
        uint32_t pulseOnMs;
        uint32_t trailingOffMs;
    };

    struct BatteryPatternTiming
    {
        uint32_t startPulseOnMs;
        uint32_t startPulseOffMs;
        uint32_t fullOnMs;
        uint32_t fullOffMs;
        uint32_t segmentOnMs;
        uint32_t segmentOffMs;
        uint32_t zeroPercentPauseMs;
    };

    struct PersistentPatternTiming
    {
        uint32_t idleWaitMs;
        uint32_t communicationOnMs;
        uint32_t communicationOffMs;
        uint32_t sensorOnMs;
        uint32_t sensorOffMs;
    };

    struct SensorPrepOverlayTiming
    {
        uint32_t totalDurationMs;
        uint32_t fadeDirectionDurationMs;
    };

    // ---------------------------------------------------------------------
    // States for recurring patterns (communication / sensor preparation).
    // ---------------------------------------------------------------------
    const uint8_t PERSISTENT_IDLE = 0;
    const uint8_t PERSISTENT_COMMUNICATION = 1;
    const uint8_t PERSISTENT_SENSOR_PREP = 2;

    // ---------------------------------------------------------------------
    // States for one-off overlays (e.g., error or boot blinking).
    // ---------------------------------------------------------------------
    const uint8_t OVERLAY_NONE = 0;
    const uint8_t OVERLAY_BOOT = 1;
    const uint8_t OVERLAY_ERROR = 2;
    const uint8_t OVERLAY_BATTERY = 3;
    const uint8_t OVERLAY_MEASUREMENT = 4;
    const uint8_t OVERLAY_SENSOR_PREP = 5;

    // Global variables tracked by the background task.
    TaskHandle_t ledTaskHandle = nullptr; // Handle for the task (not strictly required).

    uint8_t persistentState = PERSISTENT_IDLE; // Remembers the ongoing blink pattern.
    uint8_t overlayState = OVERLAY_NONE;       // Remembers one-off sequences.

    bool communicationActive = false; // Is the communication pattern needed?
    bool sensorPrepActive = false;    // Is the sensor preparation pattern needed?

    uint8_t batteryPercent = 0;                // Last reported battery level.
    void (*errorCompleteCallback)() = nullptr; // Optional callback after error blinking.

    bool initialized = false; // Has initStatusLed() been called?

    // Has a new command been reported since the last loop iteration?
    volatile bool commandPending = false;

    BootPatternTiming bootTiming{3, 1000, 100};
    ErrorPatternTiming errorTiming{60000, 20};
    MeasurementPatternTiming measurementTiming{100, 0};
    BatteryPatternTiming batteryTiming{100, 100, 5000, 1000, 1000, 1000, 1000};
    PersistentPatternTiming persistentTiming{10, 250, 250, 2000, 1000};
    SensorPrepOverlayTiming sensorPrepOverlayTiming{0, 3000};

    // ---------------------------------------------------------------------
    // Helper functions
    // ---------------------------------------------------------------------

    // Turns the LED on or off.
    void setLed(bool on)
    {
        ledcWrite(LEDC_CHANNEL, on ? LED_MAX_DUTY : 0);
    }

    void setLedBrightness(uint16_t duty)
    {
        if (duty > LED_MAX_DUTY)
        {
            duty = LED_MAX_DUTY;
        }
        ledcWrite(LEDC_CHANNEL, duty);
    }

    // Updates the persistent state according to the two flags.
    void updatePersistentState()
    {
        if (sensorPrepActive)
        {
            persistentState = PERSISTENT_SENSOR_PREP;
        }
        else if (communicationActive)
        {
            persistentState = PERSISTENT_COMMUNICATION;
        }
        else
        {
            persistentState = PERSISTENT_IDLE;
        }
    }

    void notifyCommandUpdate()
    {
        commandPending = true;
    }

    bool consumeCommandUpdate()
    {
        if (commandPending)
        {
            commandPending = false;
            return true;
        }

        return false;
    }

    // Waits for a new command. The current pattern keeps running while waiting.
    // Returns "true" if a new command arrived during the wait.
    bool waitForCommand(uint32_t waitMs)
    {
        if (consumeCommandUpdate())
        {
            return true;
        }

        if (waitMs == 0)
        {
            return false;
        }

        TickType_t waitTicks = pdMS_TO_TICKS(waitMs);
        if (waitTicks == 0)
        {
            waitTicks = 1; // Wait at least one tick.
        }

        TickType_t pollDelay = pdMS_TO_TICKS(10);
        if (pollDelay == 0)
        {
            pollDelay = 1;
        }

        TickType_t startTick = xTaskGetTickCount();

        while ((xTaskGetTickCount() - startTick) < waitTicks)
        {
            if (consumeCommandUpdate())
            {
                return true;
            }

            vTaskDelay(pollDelay);
        }

        return false;
    }

    // Makes the LED flash once (on -> wait -> off -> wait).
    // Aborts immediately once a new command arrives.
    bool blinkOnce(uint32_t onMs, uint32_t offMs)
    {
        setLed(true);
        if (waitForCommand(onMs))
        {
            setLed(false);
            return true;
        }

        setLed(false);
        if (waitForCommand(offMs))
        {
            return true;
        }

        return false;
    }

    // Repeats blinkOnce() multiple times.
    bool blinkSequence(uint8_t count, uint32_t onMs, uint32_t offMs)
    {
        for (uint8_t i = 0; i < count; ++i)
        {
            if (blinkOnce(onMs, offMs))
            {
                return true; // A new command arrived.
            }
        }
        return false;
    }

    // Blink pattern: 3 long flashes.
    bool runBootPattern()
    {
        if (blinkSequence(bootTiming.blinkCount, bootTiming.onMs, bootTiming.offMs))
        {
            return true;
        }

        if (overlayState == OVERLAY_BOOT)
        {
            overlayState = OVERLAY_NONE;
        }
        return false;
    }

    // Blink pattern: slowly fade up and down for 60 seconds.
    bool runErrorPattern()
    {
        uint32_t elapsedMs = 0;

        while (elapsedMs < errorTiming.totalDurationMs)
        {
            for (uint16_t duty = 0; duty <= LED_MAX_DUTY && elapsedMs < errorTiming.totalDurationMs; ++duty)
            {
                setLedBrightness(duty);

                uint32_t waitDuration = errorTiming.stepDurationMs;
                if (elapsedMs + waitDuration > errorTiming.totalDurationMs)
                {
                    waitDuration = errorTiming.totalDurationMs - elapsedMs;
                }

                if (waitDuration > 0 && waitForCommand(waitDuration))
                {
                    setLed(false);
                    return true;
                }

                elapsedMs += waitDuration;
            }

            for (int duty = static_cast<int>(LED_MAX_DUTY); duty >= 0 && elapsedMs < errorTiming.totalDurationMs; --duty)
            {
                setLedBrightness(static_cast<uint16_t>(duty));

                uint32_t waitDuration = errorTiming.stepDurationMs;
                if (elapsedMs + waitDuration > errorTiming.totalDurationMs)
                {
                    waitDuration = errorTiming.totalDurationMs - elapsedMs;
                }

                if (waitDuration > 0 && waitForCommand(waitDuration))
                {
                    setLed(false);
                    return true;
                }

                elapsedMs += waitDuration;
            }
        }

        setLed(false);

        if (overlayState == OVERLAY_ERROR)
        {
            overlayState = OVERLAY_NONE;
        }

        if (errorCompleteCallback)
        {
            errorCompleteCallback();
        }

        return false;
    }

    bool runSensorPrepPattern()
    {
        if (sensorPrepOverlayTiming.totalDurationMs == 0)
        {
            if (overlayState == OVERLAY_SENSOR_PREP)
            {
                overlayState = OVERLAY_NONE;
            }
            setLed(false);
            return false;
        }

        uint32_t fadeDuration = sensorPrepOverlayTiming.fadeDirectionDurationMs;
        if (fadeDuration == 0)
        {
            fadeDuration = 3000;
        }

        const uint16_t stepCount = static_cast<uint16_t>(LED_MAX_DUTY) + 1;
        uint32_t baseStepDuration = fadeDuration / stepCount;
        uint32_t remainderStepDuration = fadeDuration % stepCount;
        if (baseStepDuration == 0 && remainderStepDuration == 0)
        {
            baseStepDuration = 1;
        }

        uint32_t totalElapsed = 0;

        auto nextStepDuration = [&](uint16_t stepIndex) -> uint32_t
        {
            uint32_t duration = baseStepDuration;
            if (remainderStepDuration > 0 && stepIndex < remainderStepDuration)
            {
                ++duration;
            }
            if (duration == 0)
            {
                duration = 1;
            }
            return duration;
        };

        while (totalElapsed < sensorPrepOverlayTiming.totalDurationMs)
        {
            uint32_t directionElapsed = 0;
            uint16_t stepIndex = 0;

            for (uint16_t duty = 0; duty <= LED_MAX_DUTY && totalElapsed < sensorPrepOverlayTiming.totalDurationMs; ++duty, ++stepIndex)
            {
                if (directionElapsed >= fadeDuration)
                {
                    break;
                }
                setLedBrightness(duty);

                uint32_t waitDuration = nextStepDuration(stepIndex);
                if (directionElapsed + waitDuration > fadeDuration)
                {
                    waitDuration = fadeDuration - directionElapsed;
                }
                if (totalElapsed + waitDuration > sensorPrepOverlayTiming.totalDurationMs)
                {
                    waitDuration = sensorPrepOverlayTiming.totalDurationMs - totalElapsed;
                }

                if (waitDuration > 0 && waitForCommand(waitDuration))
                {
                    setLed(false);
                    return true;
                }

                totalElapsed += waitDuration;
                directionElapsed += waitDuration;
            }

            directionElapsed = 0;
            stepIndex = 0;

            for (int duty = static_cast<int>(LED_MAX_DUTY); duty >= 0 && totalElapsed < sensorPrepOverlayTiming.totalDurationMs; --duty, ++stepIndex)
            {
                if (directionElapsed >= fadeDuration)
                {
                    break;
                }
                setLedBrightness(static_cast<uint16_t>(duty));

                uint32_t waitDuration = nextStepDuration(stepIndex);
                if (directionElapsed + waitDuration > fadeDuration)
                {
                    waitDuration = fadeDuration - directionElapsed;
                }
                if (totalElapsed + waitDuration > sensorPrepOverlayTiming.totalDurationMs)
                {
                    waitDuration = sensorPrepOverlayTiming.totalDurationMs - totalElapsed;
                }

                if (waitDuration > 0 && waitForCommand(waitDuration))
                {
                    setLed(false);
                    return true;
                }

                totalElapsed += waitDuration;
                directionElapsed += waitDuration;
            }
        }

        setLed(false);

        if (overlayState == OVERLAY_SENSOR_PREP)
        {
            overlayState = OVERLAY_NONE;
        }

        sensorPrepOverlayTiming.totalDurationMs = 0;

        return false;
    }

    // Blink pattern: short pulse.
    bool runMeasurementPattern()
    {
        setLed(true);
        if (waitForCommand(measurementTiming.pulseOnMs))
        {
            setLed(false);
            return true;
        }

        setLed(false);
        if (measurementTiming.trailingOffMs > 0 && waitForCommand(measurementTiming.trailingOffMs))
        {
            return true;
        }
        if (overlayState == OVERLAY_MEASUREMENT)
        {
            overlayState = OVERLAY_NONE;
        }

        return false;
    }

    // Blink pattern: battery indicator.
    bool runBatteryPattern()
    {
        uint8_t clamped = batteryPercent > 100 ? 100 : batteryPercent;

        if (blinkOnce(batteryTiming.startPulseOnMs, batteryTiming.startPulseOffMs))
        {
            return true;
        }

        if (clamped >= 100)
        {
            // 100%: LED stays on for 5 seconds, then off for 1 second.
            setLed(true);
            if (waitForCommand(batteryTiming.fullOnMs))
            {
                setLed(false);
                return true;
            }
            setLed(false);
            if (waitForCommand(batteryTiming.fullOffMs))
            {
                return true;
            }
        }
        else
        {
            // Divide by 10 so we show a maximum of 9 blink segments.
            uint8_t segments = clamped / 10;
            if (segments == 0)
            {
                if (waitForCommand(batteryTiming.zeroPercentPauseMs))
                {
                    return true;
                }
            }
            else
            {
                if (segments > 9)
                {
                    segments = 9;
                }

                for (uint8_t i = 0; i < segments; ++i)
                {
                    setLed(true);
                    if (waitForCommand(batteryTiming.segmentOnMs))
                    {
                        setLed(false);
                        return true;
                    }
                    setLed(false);
                    if (waitForCommand(batteryTiming.segmentOffMs))
                    {
                        return true;
                    }
                }
            }
        }

        if (blinkOnce(batteryTiming.startPulseOnMs, batteryTiming.startPulseOffMs))
        {
            return true;
        }

        if (overlayState == OVERLAY_BATTERY)
        {
            overlayState = OVERLAY_NONE;
        }
        return false;
    }

    // Persistent patterns while no overlay is active.
    void runPersistentPattern()
    {
        if (persistentState == PERSISTENT_IDLE)
        {
            // Nothing to do: LED stays off while we patiently wait for the next command.
            setLed(false);
            waitForCommand(persistentTiming.idleWaitMs);
        }
        else if (persistentState == PERSISTENT_COMMUNICATION)
        {
            // LED blinks quickly: 250 ms on, 250 ms off.
            setLed(true);
            if (waitForCommand(persistentTiming.communicationOnMs))
            {
                setLed(false);
                return;
            }
            setLed(false);
            if (waitForCommand(persistentTiming.communicationOffMs))
            {
                return;
            }
        }
        else if (persistentState == PERSISTENT_SENSOR_PREP)
        {
            // LED blinks slowly: 2 seconds on, 1 second off.
            setLed(true);
            if (waitForCommand(persistentTiming.sensorOnMs))
            {
                setLed(false);
                return;
            }
            setLed(false);
            if (waitForCommand(persistentTiming.sensorOffMs))
            {
                return;
            }
        }
    }

    // The FreeRTOS task: started once and then runs forever.
    void ledTask(void *)
    {
        setLed(false);

        for (;;)
        {
            if (overlayState != OVERLAY_NONE)
            {
                // Overlays have priority: as long as one is active it is processed.
                bool interrupted = false;

                if (overlayState == OVERLAY_BOOT)
                {
                    interrupted = runBootPattern();
                }
                else if (overlayState == OVERLAY_ERROR)
                {
                    interrupted = runErrorPattern();
                }
                else if (overlayState == OVERLAY_BATTERY)
                {
                    interrupted = runBatteryPattern();
                }
                else if (overlayState == OVERLAY_MEASUREMENT)
                {
                    interrupted = runMeasurementPattern();
                }
                else if (overlayState == OVERLAY_SENSOR_PREP)
                {
                    interrupted = runSensorPrepPattern();
                }

                if (interrupted)
                {
                    // A new command arrived -> restart loop to handle it immediately.
                    continue;
                }
            }
            else
            {
                // No special pattern active -> play the persistent pattern.
                runPersistentPattern();
            }
        }
    }
}

// -----------------------------------------------------------------------------
// Public functions (see led.h)
// -----------------------------------------------------------------------------

void initStatusLed()
{
    if (initialized)
    {
        return; // Already initialized.
    }

    ledcSetup(LEDC_CHANNEL, LEDC_FREQUENCY, LEDC_RESOLUTION);
    ledcAttachPin(STATUS_LED_PIN, LEDC_CHANNEL);
    setLed(false);

    BaseType_t result = xTaskCreatePinnedToCore(ledTask, "led_status", 2048, nullptr, 1, &ledTaskHandle, 1);
    if (result == pdPASS)
    {
        initialized = true;
    }
}

void ledSignalBoot()
{
    // Customize boot pattern timing here.
    bootTiming = {3, 1000, 100};
    overlayState = OVERLAY_BOOT;
    notifyCommandUpdate();
}

void ledSignalError()
{
    // Customize error pattern timing here.
    errorTiming = {60000, 20};
    overlayState = OVERLAY_ERROR;
    notifyCommandUpdate();
}

void ledSignalBattery(uint8_t percent)
{
    batteryPercent = percent;
    // Customize battery pattern timing here.
    batteryTiming = {100, 100, 5000, 1000, 1000, 1000, 1000};
    overlayState = OVERLAY_BATTERY;
    notifyCommandUpdate();
}

void ledSignalMeasurement()
{
    // Customize measurement pattern timing here.
    measurementTiming = {10, 0};
    overlayState = OVERLAY_MEASUREMENT;
    notifyCommandUpdate();
}

void ledSetCommunicationActive(bool active)
{
    // Customize communication pattern timing here.
    persistentTiming.communicationOnMs = 250;
    persistentTiming.communicationOffMs = 250;
    persistentTiming.idleWaitMs = 10;
    communicationActive = active;
    updatePersistentState();
    notifyCommandUpdate();
}

void ledSetSensorPrepActive(bool active)
{
    // Customize sensor preparation pattern timing here.
    persistentTiming.sensorOnMs = 2000;
    persistentTiming.sensorOffMs = 1000;
    persistentTiming.idleWaitMs = 10;
    sensorPrepActive = active;
    updatePersistentState();
    notifyCommandUpdate();
}

void ledSignalSensorPreparation(uint32_t durationMs)
{
    if (durationMs == 0)
    {
        return;
    }

    sensorPrepOverlayTiming.totalDurationMs = durationMs;
    sensorPrepOverlayTiming.fadeDirectionDurationMs = 300;
    overlayState = OVERLAY_SENSOR_PREP;
    notifyCommandUpdate();
}

void ledRegisterErrorCompleteCallback(void (*callback)())
{
    errorCompleteCallback = callback;
}

void ledResetAllStates()
{
    initialized = false;
    communicationActive = false;
    sensorPrepActive = false;
    overlayState = OVERLAY_NONE;
    batteryPercent = 0;
    sensorPrepOverlayTiming.totalDurationMs = 0;

    updatePersistentState();
    setLed(false);
    notifyCommandUpdate();
}

