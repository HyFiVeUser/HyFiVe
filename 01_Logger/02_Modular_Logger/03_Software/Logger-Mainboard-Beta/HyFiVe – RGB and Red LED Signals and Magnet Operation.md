# HyFiVe – RGB and Red LED Signals and Magnet Operation

Technical reference for both supported LED hardware variants:

The blink patterns are identical on both variants. The visible color and the
magnet duration required to start a configuration update are the only normal
operating differences.

## Legend

| Symbol | Meaning |
|---|---:|
| `*` | LED ON: 0.2 seconds |
| `-` | LED ON: 0.6 seconds |
| `_` | LED ON: 2 seconds |
| Gap between blink pulses | LED OFF: 0.2 seconds |
| Standard pause after a signal | LED OFF: 3 seconds |

## Color categories

With the default RGB configuration `ledColorConfig = 241`, the hardware
variants display the following colors:

| Signal category | Meaning | RGB hardware | Red-only hardware |
|---|---|:---:|:---:|
| Normal operation | The logger is operating as intended. | 🟢 Green | 🔴 Red |
| Interaction and system | A user action or system process is in progress. | 🔵 Blue | 🔴 Red |
| Warning or error | The logger requires attention. | 🔴 Red | 🔴 Red |

### RGB color coding in the configuration interface

This section applies only to RGB hardware.

| Color | Code |
|---|---:|
| Red | `1` |
| Green | `2` |
| Yellow | `3` |
| Blue | `4` |
| Magenta | `5` |
| Cyan | `6` |
| White | `7` |

The RGB colors are specified as a three-digit number:

| Digit | Signal category |
|---:|---|
| 1st digit | Normal operation (`normal`) |
| 2nd digit | Interaction and system (`interaction`) |
| 3rd digit | Warnings and errors (`errors`) |

Example: `ledColorConfig = 241`

- `2` = normal operation in green
- `4` = interaction and system in blue
- `1` = warnings and errors in red

The colors can be changed in the configuration. The assignment of signals to normal operation, interaction, and errors remains unchanged.

## Magnet operation

While the magnet is detected, the LED flashes once per second:
0.2 seconds ON and 0.8 seconds OFF.

The action is triggered when the magnet is removed. Only the hardware restart
after 25 seconds is performed while the magnet is still being held in place.

### Magnet timing by hardware variant

| Magnet duration | RGB hardware | Red-only hardware |
|---:|---|---|
| less than 1 s | No action | No action |
| 1 to less than 2 s | Wake up / no action if already active | Wake up / no action if already active |
| 2 to less than 5 s | Wake up / no action if already active | Start configuration update |
| 5 to less than 10 s | Start configuration update | Hardware restart |
| 10 to less than 15 s | Software restart | - |
| 15 to 20 s inclusive | Deep Sleep | - |
| more than 20 to less than 25 s | No action | - |
| 25 s or longer | Hardware restart | - |

### Waking the logger from Deep Sleep

1. Hold the magnet against the reed switch.
2. Wait for the immediate LED response.
3. Remove the magnet.

If the logger does not respond, it may be completely without power or its
battery may be empty. In this case, connect the power supply or charger.
Connecting the power supply is also configured as a wake-up source.

## Normal operation

| No. | Priority | RGB color | Red-only color | Event | Blink pattern | Signal duration | Meaning / recommended action |
|---:|---:|:---:|:---:|---|---|---|---|
| 1 | 1 | 🟢 | 🔴 | Magnet detected | `*`, once per second | 0.2 s ON + 0.8 s OFF | Hold the magnet in place for the required action. |
| 2 | 6 | 🟢 | 🔴 | Logger active | `*` | 0.2 s ON + 0.8 s OFF | The logger is operating normally. |
| 3 | 2 | 🟢 | 🔴 | Start of a measurement trip detected | `*`, repeated | 0.2 s ON + 0.2 s OFF, repeated for **30.0 s** | Sensor recording is being prepared. |
| 4 | 2 | 🟢 | 🔴 | Measurement trip in progress | `*` | 0.2 s ON + 0.2 s OFF | Normal measurement operation. |
| 5 | 2 | 🟢 | 🔴 | End of a measurement trip detected | `- -` | 2 × 0.6 s ON + 2 × 0.2 s OFF | The measurement trip has ended. |
| 6 | 2 | 🟢 | 🔴 | Data transfer completed | `_` | 2 s ON + 3 s pause | The data was transferred successfully. |
| 7 | 5 | 🟢 | 🔴 | Battery fully charged | `_`, repeated while the power supply is connected | 2 s ON + 3 s pause | The charger can be disconnected. |

## Interaction and system

| No. | Priority | RGB color | Red-only color | Event | Blink pattern | Signal duration | Meaning / recommended action |
|---:|---:|:---:|:---:|---|---|---|---|
| 8 | 5 | 🔵 | 🔴 | Battery charging | `*` | 0.2 s ON + 3 s pause | Charging is in progress. |
| 9 | 2 | 🔵 | 🔴 | Configuration update starts | `* *` | 2 × 0.2 s ON + 0.2 s OFF + 3 s pause | Leave the logger switched on and do not interrupt the process. |
| 10 | 2 | 🔵 | 🔴 | Restart begins | `- -` | 2 × 0.6 s ON + 0.2 s OFF + 3 s pause | Wait until the logger is ready again. |
| 11 | 4 | 🔵 | 🔴 | Logger busy | `- *`, repeated | 0.6 s ON + 0.2 s OFF + 0.2 s ON + 3 s pause | A background process is running. Do not switch off or interrupt the logger. |
| 12 | 2 | 🔵 | 🔴 | Startup or update completed | `_` | 2 s ON + 3 s pause | The logger is ready again. |

## Warnings and errors

| No. | Priority | RGB color | Red-only color | Event | Blink pattern | Signal duration | Meaning / recommended action |
|---:|---:|:---:|:---:|---|---|---|---|
| 13 | 4 | 🔴 | 🔴 | Battery level low (below 15%) | `* *` | 2 × 0.2 s ON + 0.2 s OFF + 3 s pause | Charge the battery soon. |
| 14 | 3 | 🔴 | 🔴 | Battery level critical (below 5%) | `- -` | 2 × 0.6 s ON + 0.2 s OFF + 3 s pause | Charge the battery as soon as possible. |
| 15 | 6 | 🔴 | 🔴 | No connection to the deck box | `* - *` | 2 × 0.2 s ON + 0.6 s ON + 2 × 0.2 s OFF + 3 s pause | Check the deck box, distance, and wireless connection. |
| 16 | 1 | 🔴 | 🔴 | Sensor skipped | RGB: error-colored version of Event 3 or 4. Red-only: same physical pattern as Event 3 or 4. | At startup: 0.2 s ON + 0.2 s OFF, repeated for 30 seconds. During deployment: 0.2 s ON + 0.2 s OFF. | Check the sensor connection, configuration, and log file. On red-only hardware this event is not visually distinguishable. |
| 17 | 1 | 🔴 | 🔴 | Fatal error | `* * * - - - * * *`, ten times | Per cycle: 6 × 0.2 s ON + 3 × 0.6 s ON + 9 × 0.2 s OFF = 4.8 s; 10 cycles | The logger restarts afterward. After the 10th cycle, the logger enters Deep Sleep. |
| 18 | 3 | 🔴 | 🔴 | Time synchronization failed | three times `* * *` | Per cycle: 3 × 0.2 s ON + 2 × 0.2 s OFF + 3 s pause = 4.0 s; 3 cycles | Check the deck box and network connection. During magnet operation, this pattern confirms Deep Sleep. |
| 19 | 3 | 🔴 | 🔴 | Configuration rejected | three times `- - -` | Per cycle: 3 × 0.6 s ON + 2 × 0.2 s OFF + 3 s pause = 5.2 s; 3 cycles | Check the configuration file. |


