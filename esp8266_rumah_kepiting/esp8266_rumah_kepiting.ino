#define BLYNK_TEMPLATE_ID "TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN "AUTH_TOKEN"

#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

// Define Temperature Sensors
#define ONE_WIRE_BUS 2 // DS18B20 Sensors is located in GPIO2 (D4)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Connect To Wifi
char ssid[] = "YOUR_SSID";
char pass[] = "YOUR_PASSWORD";

// Define Blynk
BlynkTimer timer;

void sendTempDataTimer() {
  sensors.requestTemperatures(); 
  double temperatureC = sensors.getTempCByIndex(0); // Get temperature data from sensors
  Blynk.virtualWrite(V0, temperatureC);
}

void setup(void) {
  Serial.begin(115200);
  sensors.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  // Send data to BLYNK every 1s (1000ms)
  timer.setInterval(1000L, sendTempDataTimer);
}

void loop(void) {
  Blynk.run();
  timer.run();
}
