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

## How to implement

**1. Prepare the SIM800L**
- Use an active **2G-capable** SIM with SMS credit and the **PIN lock disabled**. Check the PIN in a phone first.
- Power the SIM800L from a **3.7–4.2 V** source that can supply **2 A** bursts: a Li-ion cell or a buck converter set to 4.0 V. **Do not use the Arduino 5V or 3.3V pin.**
- Use a voltage divider on Arduino D8 → SIM800L RX: **1 kΩ in series, 2 kΩ to GND**.
- Attach the antenna. The LED should blink about **once every 3 s** once it registers on the network.

**2. Wire it**
- Follow the wiring table above, and **connect every GND together**.
- Mount the HC-SR04 at the tank top, facing straight down, clear of the inlet pipe.

**3. Measure your tank**
- `TANK_HEIGHT_CM`: distance from the sensor face to the tank bottom.
- `SENSOR_GAP_CM`: distance from the sensor face to the "full" water line. Keep it at least 5 cm, because the HC-SR04 can't read closer than about 2 cm.

**4. Configure and upload**
- Open `firmware/water_tank_gsm/water_tank_gsm.ino` in the Arduino IDE. `SoftwareSerial` is built in, so no libraries are needed.
- Set `PHONE_NUMBER` with the country code (e.g. `+91…`), the tank values from step 3, and `LOW_PERCENT` / `FULL_PERCENT`.
- Select **Arduino Uno** and upload.

**5. Test**
- Open the Serial Monitor at **9600** baud and check the level % as you change the water level, or move a board under the sensor.
- Cross a threshold and an SMS should arrive within a few seconds.

**Troubleshooting**
- *No SMS, and the LED blinks once per second:* not registered. Check the antenna, the 2G coverage, and the SIM PIN.
- *Module keeps restarting:* the supply can't deliver 2 A. Use a better source and add a 1000 µF capacitor across VCC/GND.
- *Level jumps around:* the sensor isn't level, or it's seeing the tank wall or the inlet stream.

## Example alerts
```
ALERT: Water tank LOW (18%). Please refill.
ALERT: Water tank FULL (91%). Switch off the pump.
```

## Contributing
I'm open to open-source contributions and collaboration. Issues and pull requests are welcome.
You can reach me at **vipulatluri98@gmail.com**.
