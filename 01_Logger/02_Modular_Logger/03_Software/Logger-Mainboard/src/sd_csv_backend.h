#pragma once
#include <Arduino.h>
#include "init.h"

#ifndef SD_CSV_PATH
#define SD_CSV_PATH "/measurements/datM.csv"
#endif

bool   sd_csv_begin();
void   sd_csv_set_path(const String &path);
int    sd_csv_count_all();
int    sd_csv_count_new();
String sd_csv_get_data(int start, int end, bool onlyNew);