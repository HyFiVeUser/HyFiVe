// SimplePrefs.h
#pragma once
#include <Arduino.h>
#include <Preferences.h>

// Configure your namespace and key here:
static const char *NS = "HER_";   // max. 15 characters
static const char *KEY = "ESP32"; // max. 15 characters

// Deletes -> writes a new string -> reads and prints it
// (Warning: this function always removes the existing value!)
static void prefsEraseWriteRead(const String &newValue)
{
    Preferences prefs;
    prefs.begin(NS, false);         // read/write
    prefs.remove(KEY);              // delete
    prefs.putString(KEY, newValue); // set new value
    String value = prefs.getString(KEY, "NA");
    prefs.end();

    Serial.println("prefsEraseWriteRead: ");
    Serial.println(value);
}

// Reads the stored string and prints it (or "NA" if not available)
static String setPrefs(bool silent = false)
{
    Preferences prefs;
    prefs.begin(NS, true); // true = read-only
    String value = prefs.getString(KEY, "NA");
    prefs.end();

    if (value != "000001")
    {
        prefsEraseWriteRead("000001");
    }
    return value;
}

// Reads the stored string and prints it (or "NA" if not available)
static String readPrefs(bool silent = false)
{
    Preferences prefs;
    prefs.begin(NS, true); // true = read-only
    String value = prefs.getString(KEY, "NA");
    prefs.end();

    Serial.println("readPrefs: ");
    Serial.println(value);

    return value;
}

// Sets the string ONLY if nothing exists yet (does NOT delete anything)
static void setPrefsIfEmpty(const String &newValue)
{
    Preferences prefs;
    prefs.begin(NS, false); // false = read/write
    if (!prefs.isKey(KEY))
    {
        prefs.putString(KEY, newValue);
    }
    prefs.end();
}
