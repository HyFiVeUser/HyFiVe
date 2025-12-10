/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: WiFi network connection and management
 */

#ifndef WIFINETWORK_H
#define WIFINETWORK_H

void handleNtpSynchronization();

bool connectToWifiAndSyncNTP();

#endif