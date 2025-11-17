#pragma once

#include <Arduino.h>

// This file contains simple functions for controlling a status LED.
// All functions are named so you can immediately see what they are for.
// The implementation in "led.cpp" explains every step in detail so
// beginners can follow along.

/**
 * @brief Must be called at the very beginning of your program.
 *
 * Prepares the LED and starts the background task
 * that runs through the blink patterns.
 * @return void
 */
void initStatusLed();

/**
 * @brief Flashes the LED three times to signal a successful start.
 * @return void
 */
void ledSignalBoot();

/**
 * @brief Indicates an error: blink 10 times, then pause for 60 seconds.
 * @return void
 */
void ledSignalError();

/**
 * @brief Shows the current battery level via a blink pattern.
 *
 * @param percent Battery state of charge in percent (0 to 100).
 * @return void
 */
void ledSignalBattery(uint8_t percent);

/**
 * @brief Short blink pulse that visualizes each measurement.
 * @return void
 */
void ledSignalMeasurement();

/**
 * @brief Enables or disables the blink pattern for active communication.
 * @param active true to enable the pattern, false to disable.
 * @return void
 */
void ledSetCommunicationActive(bool active);

/**
 * @brief Enables or disables the blink pattern for sensor preparation.
 * @param active true to enable the pattern, false to disable.
 * @return void
 */
void ledSetSensorPrepActive(bool active);

/**
 * @brief Plays a gentle fade up and down for sensor preparation.
 *
 * @param durationMs Total duration in milliseconds.
 * @return void
 */
void ledSignalSensorPreparation(uint32_t durationMs);

/**
 * @brief Optional: function called after the error blink pattern completes.
 * @param callback Function pointer invoked after completion.
 * @return void
 */
void ledRegisterErrorCompleteCallback(void (*callback)());

/**
 * @brief Resets all LED states and stops running patterns.
 *
 * Can be used before deep sleep so the background task
 * resumes in idle state after waking up.
 * @return void
 */
void ledResetAllStates();
