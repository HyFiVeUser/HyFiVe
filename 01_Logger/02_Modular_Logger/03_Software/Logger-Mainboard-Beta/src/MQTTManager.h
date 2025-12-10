/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: MQTT communication management
 */

#ifndef MQTTMANAGER_H
#define MQTTMANAGER_H

void processAndTransmitMeasurementData();
void moveMeasurementAndData();
void uploadStatus();
void updateConfigViaMqtt();
void handleMqttCallback(char *topic, byte *message, unsigned int length);
void requestNodeRedStatus();
void requestNodeRedBusyStatus();
void updateFWViaMqtt();

bool transmitUpdateMessage(const char *updateInfo, const char *mqtt_topic);
bool errorInloggerIdOrTimestamp();
bool isNodeRedResponsePositive();
bool connectToMqtt();
bool transmitHeaderViaMqtt();
bool transmitDataViaMqtt();
bool transmitLogViaMqtt();

#endif