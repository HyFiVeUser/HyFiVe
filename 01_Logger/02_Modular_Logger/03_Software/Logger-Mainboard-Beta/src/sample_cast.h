/*
 * CopyrightText: (C) 2024 Hensel Elektronik GmbH
 *
 * License-Identifier: MPL-2.0
 *
 * Project: Hydrography on Fishing Vessels
 * Project URL: <https://github.com/HyFiVeUser/HyFiVe>, <https://hyfive.info>
 *
 * Description: Sample casting and analysis functions
 */

#ifndef SAMPLE_CAST_H
#define SAMPLE_CAST_H

// Data structure for log entries
struct LogEntry
{
  String time;
  int pressure;
};

void createOrClearSampleFile();

bool performSampleCast();

#endif