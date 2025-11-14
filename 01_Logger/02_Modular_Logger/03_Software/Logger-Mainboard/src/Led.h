/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: LED control and status indication
 */

#ifndef LED_H
#define LED_H

void turnOnLed();
void turnOffLed();
void firstBootLedStartLED();
void bootFinishedLED();
void resetEsp32();
void setResetEspLow();
void batteryCompletelyChargedLED();
void batteryRemainingLowLED();
void loggerDetectsEndOfDeploymentLED();
void connectionOfPowerSupplyBeginChargingOfBatteriesLED();
void ledTask(void *pvParameters);
void blinkLEDForFirstMeasurements(void *pvParameters);
void loggerTransmittedMeasurementDataLED();
void generalAlarmLed();
void bootFinishedButNtpUpdateNotPossibleLED();
void firstBootLed();
void startLEDBlinkTaskForInitialMeasurements();
void waitForLEDBlinkTaskCompletion();

#endif