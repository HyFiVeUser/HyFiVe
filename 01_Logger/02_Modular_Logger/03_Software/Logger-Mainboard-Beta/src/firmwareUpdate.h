/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Firmware update management
 */

#ifndef FIRMWAREUPDATE_H
#define FIRMWAREUPDATE_H

void updateFirmware();
bool calculateSha256(String received_sha256);

#endif