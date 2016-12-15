// case study and basic routines for reading SONOFF_POW by Reinhard Nickels - please visit my blog http://glaskugelsehen.de
// for SonOFF POW set Core Dev Module, Flash 1M, 64k SPIFFS
// thanks to Xose Pérez for his great work on the HLW8012 lib and his inspiration for this code
// basic routines from ArduinoOTA lib are included

#include <HLW8012.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define DEBUG 1               // enable = 1, disable = 0
#define CALIBRATE_RUN 0       // do calibration to determine the inital values    enable = 1, disable = 0
                              // you can also switch to calibration mode when pressing the button while connecting to Wifi
// GPIOs
#define RELAY_PIN     12
#define LED_PIN       15
#define BUTTON_PIN    0
#define SEL_PIN       5
#define CF1_PIN       13
#define CF_PIN        14

#define UPDATE_TIME                     10000   // Check values every 10 seconds
#define CURRENT_MODE                    HIGH
// These are the nominal values for the resistors in the circuit
#define CURRENT_RESISTOR                0.001
#define VOLTAGE_RESISTOR_UPSTREAM       ( 5 * 470000 ) // Real: 2280k
#define VOLTAGE_RESISTOR_DOWNSTREAM     ( 1000 ) // Real 1.009k

HLW8012 hlw8012;

const char* ssid = "<your SSID>";
const char* password = "<your Wifi passcode>";

boolean run_cal =  CALIBRATE_RUN;

WiFiClient client;
const char* host = "192.168.2.140";   // TCP Client for websoccket connection
const int httpPort = 5012;
unsigned long lastmillis;
int counter;
unsigned long buttontimer;
boolean wait_for_brelease = false;

void hlw8012_cf1_interrupt() {
  hlw8012.cf1_interrupt();
}
void hlw8012_cf_interrupt() {
  hlw8012.cf_interrupt();
}

// Library expects an interrupt on both edges
void setInterrupts() {
  attachInterrupt(CF1_PIN, hlw8012_cf1_interrupt, CHANGE);
  attachInterrupt(CF_PIN, hlw8012_cf_interrupt, CHANGE);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));    // toggle
    if (!digitalRead(BUTTON_PIN)) run_cal = true;
  }
  if (run_cal) {
    digitalWrite(RELAY_PIN, HIGH);   // standard for CAL is relais = ON
  }
  else {
    digitalWrite(RELAY_PIN, LOW);
  }
  digitalWrite(LED_PIN, HIGH);

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  ArduinoOTA.onStart([]() {
    Serial.println("Start updating");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  if (DEBUG) {
    Serial.print("connecting to ");
    Serial.println(host);
    if (!client.connect(host, httpPort)) {
      Serial.println("connection failed");
      return;
    }
    client.println("[DEBUG] Connection opened");
  }
  // setting the determined value from calibration run
  if (!run_cal) {
    hlw8012.begin(CF_PIN, CF1_PIN, SEL_PIN, CURRENT_MODE, true);
    // fill in here your values found while calibrating
    hlw8012.setCurrentMultiplier(15409.09);   // change to determined values
    hlw8012.setVoltageMultiplier(431876.50);
    hlw8012.setPowerMultiplier(11791717.38);
    client.print("[HLW] current multiplier : "); client.println(hlw8012.getCurrentMultiplier());
    client.print("[HLW] voltage multiplier : "); client.println(hlw8012.getVoltageMultiplier());
    client.print("[HLW] power multiplier   : "); client.println(hlw8012.getPowerMultiplier());
  }
  if (run_cal) {        // calibration doesn't work with interrupt, needs to be solved
    hlw8012.begin(CF_PIN, CF1_PIN, SEL_PIN, CURRENT_MODE, false, 500000);
    hlw8012.setResistors(CURRENT_RESISTOR, VOLTAGE_RESISTOR_UPSTREAM, VOLTAGE_RESISTOR_DOWNSTREAM);
    // Show default (as per datasheet) multipliers
    client.print("[HLW] Default current multiplier : "); client.println(hlw8012.getCurrentMultiplier());
    client.print("[HLW] Default voltage multiplier : "); client.println(hlw8012.getVoltageMultiplier());
    client.print("[HLW] Default power multiplier   : "); client.println(hlw8012.getPowerMultiplier());
    client.println("going to cal");
    calibrate();
    ESP.restart();
  }
  setInterrupts();
}

void loop() {
  ArduinoOTA.handle();
  if (millis() - lastmillis > UPDATE_TIME) {
    if (DEBUG) {
      // client.print("[DEBUG] sending ");
    }
    client.print("[HLW] Active Power (W)    : "); client.println(hlw8012.getActivePower());
    client.print("[HLW] Voltage (V)         : "); client.println(hlw8012.getVoltage());
    client.print("[HLW] Current (A)         : "); client.println(hlw8012.getCurrent());
    client.print("[HLW] Apparent Power (VA) : "); client.println(hlw8012.getApparentPower());
    client.print("[HLW] Power Factor (%)    : "); client.println((int) (100 * hlw8012.getPowerFactor()));
    client.println();
    // When not using interrupts we have to manually switch to current or voltage monitor
    // This means that every time we get into the conditional we only update one of them
    // while the other will return the cached value.
    // hlw8012.toggleMode();
    lastmillis = millis();
  }
  // check if button was pressed, debounce and toggle relais
  if (!digitalRead(BUTTON_PIN) && buttontimer == 0 && !wait_for_brelease) {
    buttontimer = millis();
  }
  else if (!digitalRead(BUTTON_PIN) && buttontimer != 0) {
    if (millis() - buttontimer > 20 && !wait_for_brelease) {
      digitalWrite(RELAY_PIN, !digitalRead(RELAY_PIN));
      client.println("[DEBUG] Relais toggled");
      buttontimer = 0;
      wait_for_brelease = true;
    }
  }
  if (wait_for_brelease) {
    if (digitalRead(BUTTON_PIN)) wait_for_brelease = false;
  }
}

void calibrate() {
  // Let's first read power, current and voltage
  // with an interval in between to allow the signal to stabilise:
  hlw8012.getActivePower();

  hlw8012.setMode(MODE_CURRENT);
  unblockingDelay(2000);
  hlw8012.getCurrent();

  hlw8012.setMode(MODE_VOLTAGE);
  unblockingDelay(2000);
  hlw8012.getVoltage();

  // Calibrate using a 60W bulb (pure resistive) on a 230V line
  hlw8012.expectedActivePower(57.0);
  hlw8012.expectedVoltage(223.0);
  hlw8012.expectedCurrent(57.0 / 223.0);
  // Show corrected factors
  client.print("[HLW] New current multiplier : "); client.println(hlw8012.getCurrentMultiplier());
  client.print("[HLW] New voltage multiplier : "); client.println(hlw8012.getVoltageMultiplier());
  client.print("[HLW] New power multiplier   : "); client.println(hlw8012.getPowerMultiplier());
  client.println("Calibration done");
  client.println("Reflash with determined values in the code ");
}

void unblockingDelay(unsigned long mseconds) {
  unsigned long timeout = millis();
  while ((millis() - timeout) < mseconds) delay(1);
}


