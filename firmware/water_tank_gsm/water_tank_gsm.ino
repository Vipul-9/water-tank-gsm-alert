/*
 * Water Tank Level Monitoring with SMS Alerts
 *
 * HC-SR04 ultrasonic sensor measures the distance to the water surface,
 * converts it to a fill percentage, and a SIM800L GSM module sends an SMS
 * when the tank goes LOW or FULL. Hysteresis prevents repeated alerts.
 *
 * Board: Arduino Uno
 * Wiring:
 *   HC-SR04  TRIG -> D9, ECHO -> D10, VCC -> 5V, GND -> GND
 *   SIM800L  TX   -> D7 (Arduino RX), RX -> D8 (Arduino TX, via divider to ~2.8V)
 *            VCC  -> 3.7-4.2V supply that can deliver 2A bursts (NOT the Uno 5V pin)
 *            GND  -> common GND
 */

#include <SoftwareSerial.h>

// ---------------- User configuration ----------------
const char PHONE_NUMBER[] = "+91XXXXXXXXXX";  // alert recipient
const float TANK_HEIGHT_CM  = 100.0;  // sensor face to tank bottom
const float SENSOR_GAP_CM   = 5.0;    // sensor face to "full" water line
const int   LOW_PERCENT     = 20;     // alert when level drops below this
const int   FULL_PERCENT    = 90;     // alert when level rises above this
const int   HYSTERESIS      = 5;      // % margin before re-arming an alert
const unsigned long READ_INTERVAL_MS = 2000;
// ----------------------------------------------------

const int TRIG_PIN = 9;
const int ECHO_PIN = 10;
SoftwareSerial gsm(7, 8);  // RX, TX

enum TankState { NORMAL, LOW_LEVEL, FULL_LEVEL };
TankState state = NORMAL;
unsigned long lastRead = 0;

// ---------------- Ultrasonic ----------------
float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000UL);  // 30 ms timeout (~5 m)
  if (duration == 0) return -1;                               // no echo
  return duration * 0.0343 / 2.0;                             // speed of sound 343 m/s
}

// Median of 5 readings rejects ripples and stray echoes
float readDistanceFiltered() {
  float r[5];
  int n = 0;
  for (int i = 0; i < 5; i++) {
    float d = readDistanceCm();
    if (d > 0) r[n++] = d;
    delay(60);
  }
  if (n == 0) return -1;
  for (int i = 1; i < n; i++) {            // insertion sort
    float k = r[i]; int j = i - 1;
    while (j >= 0 && r[j] > k) { r[j + 1] = r[j]; j--; }
    r[j + 1] = k;
  }
  return r[n / 2];
}

int levelPercent(float distanceCm) {
  float waterHeight = TANK_HEIGHT_CM - distanceCm;
  float usable = TANK_HEIGHT_CM - SENSOR_GAP_CM;
  int pct = (int)(waterHeight / usable * 100.0);
  return constrain(pct, 0, 100);
}

// ---------------- GSM ----------------
bool waitFor(const char *token, unsigned long timeoutMs) {
  unsigned long start = millis();
  String buf;
  while (millis() - start < timeoutMs) {
    while (gsm.available()) {
      char c = gsm.read();
      Serial.write(c);
      buf += c;
      if (buf.indexOf(token) >= 0) return true;
      if (buf.length() > 128) buf.remove(0, 64);
    }
  }
  return false;
}

String readFor(unsigned long ms) {
  unsigned long start = millis();
  String buf;
  while (millis() - start < ms) {
    while (gsm.available()) {
      char c = gsm.read();
      Serial.write(c);
      if (buf.length() < 128) buf += c;
    }
  }
  return buf;
}

bool sendCommand(const char *cmd, const char *expect, unsigned long timeoutMs) {
  gsm.println(cmd);
  return waitFor(expect, timeoutMs);
}

void initGSM() {
  Serial.println(F("Initialising GSM..."));
  for (int i = 0; i < 10 && !sendCommand("AT", "OK", 1000); i++) delay(500);
  sendCommand("ATE0", "OK", 1000);        // echo off
  sendCommand("AT+CMGF=1", "OK", 1000);   // SMS text mode
  // Wait for network registration (home or roaming)
  for (int i = 0; i < 30; i++) {
    gsm.println("AT+CREG?");
    String r = readFor(1000);
    if (r.indexOf("+CREG: 0,1") >= 0 || r.indexOf("+CREG: 0,5") >= 0) {
      Serial.println(F("\nGSM registered"));
      return;
    }
    delay(1000);
  }
  Serial.println(F("\nGSM not registered - check SIM/antenna/power"));
}

bool sendSMS(const String &msg) {
  gsm.print("AT+CMGS=\"");
  gsm.print(PHONE_NUMBER);
  gsm.println("\"");
  if (!waitFor(">", 5000)) return false;
  gsm.print(msg);
  gsm.write(26);                          // Ctrl+Z sends the message
  bool ok = waitFor("+CMGS", 15000);
  Serial.println(ok ? F("\nSMS sent") : F("\nSMS failed"));
  return ok;
}

// ---------------- Main ----------------
void setup() {
  Serial.begin(9600);
  gsm.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  delay(3000);                            // let the GSM module boot
  initGSM();
  sendSMS("Water tank monitor online.");
}

void loop() {
  if (millis() - lastRead < READ_INTERVAL_MS) return;
  lastRead = millis();

  float d = readDistanceFiltered();
  if (d < 0) {
    Serial.println(F("Sensor: no echo"));
    return;
  }
  int pct = levelPercent(d);
  Serial.print(F("Distance: ")); Serial.print(d, 1);
  Serial.print(F(" cm  Level: ")); Serial.print(pct); Serial.println(F(" %"));

  switch (state) {
    case NORMAL:
      if (pct <= LOW_PERCENT) {
        if (sendSMS("ALERT: Water tank LOW (" + String(pct) + "%). Please refill.")) state = LOW_LEVEL;
      } else if (pct >= FULL_PERCENT) {
        if (sendSMS("ALERT: Water tank FULL (" + String(pct) + "%). Switch off the pump.")) state = FULL_LEVEL;
      }
      break;
    case LOW_LEVEL:
      if (pct > LOW_PERCENT + HYSTERESIS) state = NORMAL;
      break;
    case FULL_LEVEL:
      if (pct < FULL_PERCENT - HYSTERESIS) state = NORMAL;
      break;
  }
}
