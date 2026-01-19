# TODO Software V0.07

## Kritisch
- [x] Akku-Ladevorgang erreicht nicht immer 100 %.

## Wichtig
- [ ] anpassen der README.md (https://github.com/HyFiVeUser/HyFiVe/tree/main/01_Logger/02_Modular_Logger/03_Software)
- [x] RGB-LED Prioritäten prüfen, weil es noch Überlagerungen gibt
- [x] Zyklusabweichung prüfen: Differenz zwischen konfiguriertem und tatsächlichem Zyklus:
    Ja es gib Abweichungen.

    Jede Periode merkt sich den Zeitpunkt ihrer letzten Ausführung. 

    Nach jedem Durchlauf der loop-Schleife wird berechnet, wie viele Sekunden bis zur nächsten Ausführung jeder Periode verbleiben.

    Dadurch sind die Ausführungen nicht sekundengenau an feste Zeitpunkte gekoppelt. Wenn z. B. eine MQTT-Übertragung 120sec dauert, verschieben sich alle in dieser Zeit fälligen Perioden nach hinten und werden erst danach abgearbeitet.

    Beispiel: MQTT sendet 120sec lang Daten. Währenddessen wären z.b. wetDetPeriodeFunktion und configUpdatePeriodeFunktion fällig. 
    Da die loop-Schleife erst nach Abschluss von MQTT wieder weiterläuft, wird wetDetPeriodeFunktion und configUpdatePeriodeFunktion beim nächstmöglichen Zeitpunkt sofort auslösen da die Wartezeit ja bei beiden abgelaufen wehre.
    Und nach dem Auslösen der Perioden wird diesen die neue Periodendauer aus dem Configfile zugewiesen.  
- [x] generalError: LED-Signal soll nicht dauerhaft aktiv bleiben. Stattdessen den generalError LED-Event 10× ausführen (dauer ca.30sec), danach Software-Reboot. Wenn der Fehler nach insgesamt 2 Reboot-Versuchen weiterhin besteht, soll der Logger in Deep-Sleep gehen.
- [x] Interfaceboard: ADC-Auflösung auf 12 Bit umstellen
- [x] WetDet-Sensor weniger häufig abfragen
- [x] LED-Anzeige in separaten Thread auf zweiten Core auslagern
(Config Parameter)
- [x] Light-Sleep statt Deep-Sleep
- [x] Statusmeldungen während des Ladevorgangs alle 10min
- [x] Logger geht nach Ablauf der Zeit inactive_Measurement_periode (Config-Parameter) in Deep-Sleep, sofern keine Datenübertragung oder Messungen stattfinden.
- [x] Firmware soll kompatibel sein und sowohl eine 1-farbige als auch eine 3-farbige LED ansteuern können (wenn hwVariante() == 000001, wird die 3-farbige LED verwendet, sonst die 1-farbige).
- [x] LED Fraben über Config Parameter setzen
- [x] LED-Anzeige (LED_signals_rev1_4.xlsx)
    - [x] Magnet detected 
        - [x] 0-5 sec Logger goes active 
        - [x] 5-10 sec start Config update
        - [x] 10-15 sec Logger Software-Reboot
        - [x] 15-20 sec Logger go to DeepSleep
        - [x] up to 30 seconds Logger Hardware-Reboot (LED-Event "Magnet detected" no longer flashes)
    - [x] logger active
    - [x] Logger detects begin of deployment
    - [x] during deployment (after 1. minute)
    - [x] Logger detects end of deployment
    - [x] Transmission complete
    - [x] charging complete
    - [x] battery charging
    - [x] start config update
    - [x] start reboot
    - [x] Logger busy (background process)
    - [x] update / boot complete
    - [x] Battery low (15%)
    - [x] Battery superlow (5%)
    - [x] no connection to Deckbox
    - [x] general error
    - [x] NTP Update failed
    - [x] Config rejected
    

## Nice To Have
- [x] Loglevel (Logging/Debugging) ohne Neukompilieren über einen Config-Parameter konfigurierbar machen.

- LogLevel:
    - 0  |  Log only: LogLevelDEBUG, LogLevelINFO, LogLevelWARNING, LogLevelERROR
    - 1  |  Log only: LogLevelINFO, LogLevelWARNING, LogLevelERROR
    - 2  |  Log only: LogLevelWARNING, LogLevelERROR
    - 3  |  Log only: LogLevelERROR

    example = "LogCategory": LogLevel,  --> "logGeneral": 1,

    Add this to the config_xxx.json after “logger_id”: xx, one

            "logGeneral": 0,
            "logSensors": 0,
            "logUnderwater": 0,
            "logAboveWater": 0,
            "logBMS": 0, 
            "logCharger": 0,
            "logWiFi": 0, 
            "logMQTT": 0,
            "logSDCard": 0,
            "logRTC": 0, 
            "logPowerManagement": 0,
            "logConfiguration": 0,
            "logMeasurement": 0,

- [ ] Status des Loggers (der an den Server übertragen wird) um Fehlerausgaben erweitern.
    - Die ID könnte als Bitposition genutzt werden → eine Zahl kann mehrere aktive Stati enthalten.