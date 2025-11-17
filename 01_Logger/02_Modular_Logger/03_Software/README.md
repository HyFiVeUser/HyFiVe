
# AWI OSBK Logger

## Inhaltsverzeichnis

- [0. Logger Inhalt](#Logger-Inhalt) 
- [1. Bedienung über Magnetkontakt](#bedienung-magnetkontakt)  
  - [1.1 Logger aus dem Deep-Sleep holen (Bootphase starten)](#deep-sleep-aufwecken)  
  - [1.2 Logger in den Deep-Sleep versetzen](#deep-sleep-versetzen)  
- [2. Bootphase](#bootphase)  
- [3. loop (Hauptschleife im Messbetrieb)](#loop)  
- [4. Batterie / Akku](#batterie-akku)  
  - [4.1 Verhalten bei niedriger Batteriespannung](#niedrige-batteriespannung)  
  - [4.2 Laden des Akkus](#laden-des-akkus)  
- [5. Firmware-Update](#firmware-update)  
- [6. BLE-Services](#ble-services)  
  - [6.1 Live Data Service](#live-data-service)  
  - [6.2 SD Card / Storage Service](#sd-card-service)  
  - [6.3 Config Service](#config-service)  
  - [6.4 Time Service](#time-service)  
- [7. Daten zur Sensorkalibrierung](#daten-zur-sensorkalibrierung) 
---

<a name="Logger-Inhalt"></a>
## 0. Logger Inhalt
<img src="/media/Logger_packaged.jpg" alt="Ladekontakt" width="500">

1. Magnet
2. 19V 3A Ladegerät
3. Logger

<a name="bedienung-magnetkontakt"></a>
## 1. Bedienung über Magnetkontakt

<a name="deep-sleep-aufwecken"></a>
### 1.1 Logger aus dem Deep-Sleep holen (Bootphase starten)

1. Magnet an den Ladekontakt (+ oder −) halten.  
   <img src="/media/Charging_contact.jpg" alt="Ladekontakt" width="200">
2. Die Status-LED beginnt zu leuchten.
3. Warten, bis die LED **dreimal** hintereinander geblinkt hat.
4. Nach dem dritten Blinken wacht der Logger aus dem Deep-Sleep auf und geht in die **Bootphase** über.

> **Wichtig:** Wird der Magnet zu früh entfernt (also vor Ende des dritten Blinkens), geht der Logger wieder in den Deep-Sleep.

---

<a name="deep-sleep-versetzen"></a>
### 1.2 Logger in den Deep-Sleep versetzen

1. Magnet an den Ladekontakt (+ oder −) halten.  
2. Die Status-LED beginnt zu leuchten.
3. Warten, bis die LED **fünfmal** hintereinander geblinkt hat.  
   - Den Magneten während dieser fünf Blinkimpulse angelegt lassen.
4. Nach dem fünften Blinken kann der Magnet entfernt werden, die LED leuchtet für ca. **5 Sekunden** rot.
5. Anschließend wechselt der Logger in den **Deep-Sleep-Modus**.

> **Wichtig:** Wird der Magnet zu früh entfernt (also vor Ende des fünften Blinkens), geht der Logger **nicht** in den Deep-Sleep-Modus.

---

<a name="bootphase"></a>
## 2. Bootphase

In der Bootphase werden folgende Schritte durchgeführt:

- I²C-Bus für Sensoren wird initialisiert:
  - BMS wird initialisiert
  - RTC (z. B. DS3231) wird initialisiert
  - SD-Karte wird initialisiert
- Konfiguration wird geladen:
  - `config.json`
  - `calib_coeff.json`
- Firmware-Update wird geprüft  
  - Repository: <https://github.com/Stanislas-Klein/AWI-FW>
- Uhrzeit wird über WLAN (NTP) konfiguriert und – sofern verfügbar – gesetzt
- BLE wird mit folgenden Services gestartet:
  - Time Service
  - Config Service
  - Live Data Service
  - SD Card Service
- Falls eine `sensorPrepDurationTime` konfiguriert ist, läuft zunächst diese Vorbereitungszeit ab.  
  **Danach** startet die Messung.

---

<a name="loop"></a>
## 3. loop (Hauptschleife im Messbetrieb)

In der Hauptschleife werden zyklisch folgende Aufgaben ausgeführt:

- Batterie prüfen
- Sensoren auslesen
- Sensormessdaten auf SD-Karte speichern
- Live-Daten per BLE senden
- Magnetkontakt überwachen

---

<a name="batterie-akku"></a>
## 4. Batterie / Akku

Der Logger besitzt einen integrierten Akku (16,8 V), der von einem Batteriemanagement-System (BQ40Z80) überwacht wird.

<a name="niedrige-batteriespannung"></a>
### 4.1 Verhalten bei niedriger Batteriespannung

- Sinkt die Spannung unter einen kritischen Grenzwert (11,2 V), beendet der Logger den Messbetrieb.
- Der Logger signalisiert den Fehler über die Status-LED.

**Error-LED-Muster:**  

- Für ca. **10 Sekunden** in Schleife:
  - **50 ms AN**
  - **50 ms AUS**
- Anschließend versetzt sich der Logger automatisch in den Deep-Sleep.

---

<a name="laden-des-akkus"></a>
### 4.2 Laden des Akkus

- Der Akku wird über den Ladekontakt geladen.
- Der Logger kann im Deep-Sleep oder im Idle-Betrieb geladen werden.
- Während des Ladevorgangs werden keine Sensormessungen durchgeführt.
- Beim Laden im Idle-Betrieb wird der Ladezustand über die Status-LED mittels Blinkmustern angezeigt (siehe unten).

#### Ladezustand – LED-Blinkmuster

**Akkustand < 10 %**  

- 1 × sehr kurzer Blinkimpuls:
  - **100 ms AN**
  - **100 ms AUS**
- Davor und danach jeweils ca. **1 s AUS**.

---

**Akkustand 10–99 %**  

- Anzahl der Blinkimpulse in 10-%-Schritten:
  - 10–19 % → blinkt **1×**
  - 20–29 % → blinkt **2×**
  - 30–39 % → blinkt **3×**
  - …  
  - 90–99 % → blinkt **9×**
- Jeder dieser Blinkimpulse hat folgende Zeiten:
  - **500 ms AN**
  - **500 ms AUS**
- Davor und danach jeweils ca. **1 s AUS**.

---

**Akkustand = 100 %**  

- 1 × langer Blinkimpuls:
  - **5 s AN**
  - anschließend **1 s AUS**
- Davor und danach jeweils ca. **1 s AUS**.

> **Hinweis:** Die Blinksequenzen laufen zyklisch durch, solange das Ladegerät angeschlossen ist.

> **Wichtig:** Logger nicht über längere Zeit mit leerem Akku lagern.

> **Wichtig:** Wurde der Akku an der Elektronik getrennt, muss die  
> [BOOT BMS](https://github.com/HyFiVeUser/HyFiVe/blob/main/01_Logger/02_Modular_Logger/01_Electronics/media/Logger-Mainboard_Connection_Overview.jpg) Taste gedrückt werden, um den Logger wieder in Betrieb zu nehmen. Damit wird das BMS gestartet und der Logger ist erneut nutzbar.

---

<a name="firmware-update"></a>
## 5. Firmware-Update

- Für ein Firmware-Update ist ein WLAN-Zugang erforderlich.
- Updates werden **nur in der Bootphase** durchgeführt.
- Unterscheidet sich die `CRC32.txt` auf der SD-Karte von der im Repository,
  wird die `firmware.bin` heruntergeladen und anschließend das Firmware-Update durchgeführt.

---

<a name="ble-services"></a>
## 6. BLE-Services

Der Logger stellt insgesamt vier BLE-Services bereit:

1. Live Data Service  
2. SD Card / Storage Service  
3. Config Service  
4. Time Service  

---

<a name="live-data-service"></a>
### 6.1 Live Data Service

- **Service-UUID:** `00000000-83a3-449e-afeb-52e55909e63c`  
- **Characteristic-UUID:** `00000200-83a3-449e-afeb-52e55909e63c`  

Sendet aktuelle Messwerte in Echtzeit, Datenformat: JSON

---

<a name="sd-card-service"></a>
### 6.2 SD Card / Storage Service

- **Service-UUID:** `00020000-83a3-449e-afeb-52e55909e63c`  
- **Characteristic-UUID:** `00020100-83a3-449e-afeb-52e55909e63c`  

Überträgt gespeicherte Messdaten von der SD-Karte  

---

<a name="config-service"></a>
### 6.3 Config Service

- **Service-UUID:** `00010000-83a3-449e-afeb-52e55909e63c`  
- **Characteristic-UUID:** `00010100-83a3-449e-afeb-52e55909e63c`  

Dient zum Übertragen der Konfiguration.


---

<a name="time-service"></a>
### 6.4 Time Service

- **Service-UUID:** `00030000-83a3-449e-afeb-52e55909e63c`  
- **Characteristic-UUID:** `00030100-83a3-449e-afeb-52e55909e63c`  

Setzt die Echtzeituhr (RTC, UTC-Zeitstempel) per BLE

<a name="daten-zur-sensorkalibrierung"></a>
## 7. Daten zur Sensorkalibrierung

Für jeden Logger müssen individuelle **Kalibrierdaten** hinterlegt werden.  
Diese werden als Datei `calib_coeff.json` über den **Config Service** auf den Logger übertragen.

Beispiel für den Eintrag in der Konfiguration (z. B. in `config.json`):

```json
"pythonFileNames": ["sensor_lib.py", "calib_coeff.json"]
```

calib_coeff.json
```json
{
  "interfaceMcu": {
    "parameters": [
      {
        "id": "01",
        "name": "Conductivity",
        "calib_coeff": {}
      },
      {
        "id": "02",
        "name": "Pressure",
        "calib_coeff": {}
      },
      {
        "id": "03",
        "name": "Dissolved Oxygen",
        "calib_coeff": {}
      },
      {
        "id": "04",
        "name": "Temperature",
        "calib_coeff": {
          "1": 34031,
          "2": 22607,
          "3": 15996,
          "4": 7338,
          "5": 5714
        }
      },
      {
        "id": "05",
        "name": "Turbidity ADC-Voltage",
        "calib_coeff": {
          "1": 0.1550,
          "2": 46.8428
        }
      }
    ]
  }
}
```

---