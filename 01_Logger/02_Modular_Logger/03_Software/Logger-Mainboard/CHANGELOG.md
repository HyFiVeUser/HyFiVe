# Changelog

## V0.86

### Multi-client access control

* Added access control for multiple clients.
* Clients send a request to Node-RED to check whether the communication interface is currently busy.
* Multiple clients can issue requests, but only one client is granted exclusive access at a time.
* Other clients remain in a waiting state until the interface becomes available.

### Wi-Fi connection handling

* The logger can now automatically reconnect to the Wi-Fi network where the last successful login took place.
* Manual sorting of Wi-Fi entries in the JSON list is no longer required.
* If the most recently successful Wi-Fi connection cannot be established, the logger tries the Wi-Fi networks stored in the JSON list one after another.
* As soon as a connection is successful, that network is set as the first choice in the list.

### Charging mode

* When the battery is being charged, the logger switches to charging mode.
* In charging mode, only the battery is charged.
* The logger does not perform any other functions, except sending the `battery remaining` value to Node-RED every 10 minutes.
* Once the charging process is finished and measurement data is available, the data is transmitted.
* To use the logger for measurements again, it should be disconnected from the charger.

## V0.82

### Undervoltage protection

* Added undervoltage protection.

## V0.81

### Wi-Fi stability

* Resolved Wi-Fi connection issues.
* Firmware updated to V0.81.

## V0.80

### OTA firmware update

* Added OTA firmware update feature.
* Implemented Node-RED flow for OTA firmware updates.
* Related Node-RED flow file: `nodered_all_flows_20241120.json`

### Code improvements

* Enhanced code comments.
* Optimized function behavior.

## V0.71

### Charging behavior

* Adjusted the charging process up to 100%, as battery-dependent variations are possible.

## V0.70

### Charging stability

* Previously, errors could occur during charging if the battery was discharged below 5% due to undervoltage.
* This issue has been resolved with the current firmware update.

## V0.60

### Initial release

* Initial version.
