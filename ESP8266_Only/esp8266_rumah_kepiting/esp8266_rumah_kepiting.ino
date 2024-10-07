#define BLYNK_TEMPLATE_ID "TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN "AUTH_TOKEN"

#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <U8g2lib.h>

// TDS Sensor
#define TdsSensorPin A0
#define VREF 3.3
#define SCOUNT 30
int analogBuffer[SCOUNT];
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;

// Temperature Sensors
#define ONE_WIRE_BUS 2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature temperatureSensor(&oneWire);

// OLED Display (SH1106)
U8G2_SH1106_128X64_NONAME_F_HW_I2C oledDisplay(U8G2_R0); // Create OLED Display named oledDisplay

// OLED Icon
static const unsigned char image_weather_temperature_bits[] U8X8_PROGMEM = {0x38,0x00,0x44,0x40,0xd4,0xa0,0x54,0x40,0xd4,0x1c,0x54,
0x06,0xd4,0x02,0x54,0x02,0x54,0x06,0x92,0x1c,0x39,0x01,0x75,0x01,0x7d,0x01,0x39,0x01,0x82,0x00,0x7c,0x00};
static const unsigned char image_weather_humidity_bits[] U8X8_PROGMEM = {0x20,0x00,0x20,0x00,0x30,0x00,0x70,0x00,0x78,0x00,0xf8,0x00,0xfc,0x01,
0xfc,0x01,0x7e,0x03,0xfe,0x02,0xff,0x06,0xff,0x07,0xfe,0x03,0xfe,0x03,0xfc,0x01,0xf0,0x00};

// Connect to WiFi
char ssid[] = "YOUR_SSID";
char pass[] = "YOUR_PASSWORD";

// Blynk Timer
BlynkTimer timer;

float temperatureC = 0;  // Temperature for compensation and display
float averageVoltage = 0;
float tdsValue = 0;

// Function to calculate the median value (TDS)
int getMedianNum(int bArray[], int iFilterLen) {
  int bTab[iFilterLen];
  for (byte i = 0; i < iFilterLen; i++)
    bTab[i] = bArray[i];
  // Sort the array for median calculation
  for (int j = 0; j < iFilterLen - 1; j++) {
    for (int i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        int bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  // Return the median value
  if (iFilterLen % 2 == 0) {
    return (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  } else {
    return bTab[iFilterLen / 2];
  }
}

void readTds() {
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT) {
      analogBufferIndex = 0;
    }
  }
}

float calculateTDS() {
  // Copy buffer for processing
  for (int i = 0; i < SCOUNT; i++) {
    analogBufferTemp[i] = analogBuffer[i];
  }
  averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;

  // Use actual temperature from sensor for compensation
  float compensationCoefficient = 1.0 + 0.02 * (temperatureC - 25.0);
  float compensationVoltage = averageVoltage / compensationCoefficient;

  tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
              - 255.86 * compensationVoltage * compensationVoltage
              + 857.39 * compensationVoltage) * 0.5;
  Blynk.virtualWrite(V1, tdsValue);  // Send TDS value to Blynk
  return tdsValue;
}

void sendTempDataTimer() {
  temperatureSensor.requestTemperatures();
  temperatureC = temperatureSensor.getTempCByIndex(0);  // Update global temperature variable
  Blynk.virtualWrite(V0, temperatureC);
}

void calculateAndDisplay() {
  float tdsNow = calculateTDS();  

  char temperatureStr[10];
  char tdsStr[10];

  dtostrf(temperatureC, 4, 1, temperatureStr);
  dtostrf(tdsNow, 4, 1, tdsStr);

  oledDisplay.clearBuffer();
  oledDisplay.drawXBMP(10, 10, 16, 16, image_weather_temperature_bits);
  oledDisplay.setFont(u8g2_font_profont17_tr);
  oledDisplay.drawStr(35, 23, temperatureStr);

  oledDisplay.drawXBMP(10, 39, 11, 16, image_weather_humidity_bits);
  oledDisplay.drawStr(35, 53, tdsStr);

  oledDisplay.sendBuffer();
}

void setup(void) {
  Serial.begin(115200);

  oledDisplay.begin();
  oledDisplay.setColorIndex(1);
  oledDisplay.setFontMode(1);
  oledDisplay.setBitmapMode(1);
  oledDisplay.clearBuffer();

  temperatureSensor.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  timer.setInterval(1000L, calculateAndDisplay);
  timer.setInterval(1000L, sendTempDataTimer);
  timer.setInterval(1000L, readTds);

}

void loop(void) {
  readTds();
  Blynk.run();
  timer.run(); 
}
