# Water Tank Level Monitoring with SMS Alerts

Measures water-tank level with an ultrasonic sensor and sends **SMS alerts over GSM** when the tank runs low or becomes full.

## How it works
- An **HC-SR04** ultrasonic sensor measures the distance to the water surface. The median of 5 readings filters out ripples.
- The distance is converted to a **fill percentage** using the tank height.
- A **SIM800L GSM module** sends an SMS when the level crosses the LOW or FULL threshold.
- **Hysteresis** stops repeated alerts while the level hovers near a threshold.

```
HC-SR04 → Arduino (median filter → level %) → threshold + hysteresis → SIM800L → SMS
```

## Hardware
| Part | Notes |
|---|---|
| Arduino Uno | |
| HC-SR04 ultrasonic sensor | Mounted at the tank top, facing down |
| SIM800L GSM module + SIM | Needs a 3.7–4.2 V supply that can deliver 2 A bursts |

## Wiring
| HC-SR04 | Arduino | | SIM800L | Arduino |
|---|---|---|---|---|
| TRIG | D9 | | TX | D7 |
| ECHO | D10 | | RX | D8 (through a voltage divider) |
| VCC | 5V | | VCC | External 4 V supply |
| GND | GND | | GND | Common GND |

## Setup
1. Open `firmware/water_tank_gsm/water_tank_gsm.ino` in the Arduino IDE.
2. Set `PHONE_NUMBER`, `TANK_HEIGHT_CM`, and the `LOW_PERCENT` / `FULL_PERCENT` thresholds.
3. Upload it, insert the SIM and power on. Readings appear in the Serial Monitor at 9600 baud.

## Example alerts
```
ALERT: Water tank LOW (18%). Please refill.
ALERT: Water tank FULL (91%). Switch off the pump.
```
