// Blynk
#define BLYNK_TEMPLATE_ID "TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN "AUTH_TOKEN"

#include <OneWire.h>
#include <DallasTemperature.h> // Temperature Sensors Library
#include <ESP8266WiFi.h> // Library to connect to Wifi
#include <BlynkSimpleEsp8266.h> // Blynk Library for ESP8266
#include <U8g2lib.h> // Library for OLED Display (SH1106)

// TDS Sensor
#define TdsSensorPin A0
#define VREF 3.3
#define SCOUNT  30 
int analogBuffer[SCOUNT];
int analogBufferIndex = 0;

// Temperature Sensors
#define ONE_WIRE_BUS 2 // DS18B20 Sensors is located in GPIO2 (D4)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature temperatureSensor(&oneWire);

// OLED Display (SH1106)
U8G2_SH1106_128X64_NONAME_F_HW_I2C oledDisplay(U8G2_R0); // Create OLED Display named oledDisplay

// OLED Icon
static const unsigned char image_weather_temperature_bits[] U8X8_PROGMEM = {0x38,0x00,0x44,0x40,0xd4,0xa0,0x54,0x40,0xd4,0x1c,0x54,
0x06,0xd4,0x02,0x54,0x02,0x54,0x06,0x92,0x1c,0x39,0x01,0x75,0x01,0x7d,0x01,0x39,0x01,0x82,0x00,0x7c,0x00};
static const unsigned char image_weather_humidity_bits[] U8X8_PROGMEM = {0x20,0x00,0x20,0x00,0x30,0x00,0x70,0x00,0x78,0x00,0xf8,0x00,0xfc,0x01,
0xfc,0x01,0x7e,0x03,0xfe,0x02,0xff,0x06,0xff,0x07,0xfe,0x03,0xfe,0x03,0xfc,0x01,0xf0,0x00};

// Connect To Wifi
char ssid[] = "YOUR_SSID";
char pass[] = "YOUR_PASSWORD";

// Define Blynk Timer
BlynkTimer timer;

// Function to calculate the median value (TDS)
int getMedianNum(int bArray[], int iFilterLen) {
  std::sort(bArray, bArray + iFilterLen); // Sort the array
  if (iFilterLen % 2 == 0) {
    return (bArray[iFilterLen / 2] + bArray[iFilterLen / 2 - 1]) / 2;
  } else {
    return bArray[iFilterLen / 2];
  }
}

// Function to calculating TDS value (TDS)
float readTds() {
  // Ensure we are using the latest temperature from the sensor
  temperatureSensor.requestTemperatures(); 
  float temperature = temperatureSensor.getTempCByIndex(0);  // Update the temperature from DS18B20 sensor

  int medianValue = getMedianNum(analogBuffer, SCOUNT);
  float averageVoltage = medianValue * (float)VREF / 1024.0;
  float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);  // temperature compensation using sensor value
  float compensationVoltage = averageVoltage / compensationCoefficient;
  float tdsValue = (133.42 * pow(compensationVoltage, 3) - 255.86 * pow(compensationVoltage, 2) + 857.39 * compensationVoltage) * 0.5;

  Blynk.virtualWrite(V1, tdsValue);  // Send TDS value to Blynk on Virtual Pin V1
  return tdsValue; // Return the TDS value
}

// Reading analog value (TDS)
void readTdsAnalogSensor() {
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {  // Every 40 milliseconds, read the analog value
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);  // Read the analog value and store into the buffer
    analogBufferIndex = (analogBufferIndex + 1) % SCOUNT;  // Increment index and wrap around
  }
}

// Getting temperature data from temperature sensors (Temperature)
void sendTempDataTimer() {
  temperatureSensor.requestTemperatures(); 
  float temperatureC = temperatureSensor.getTempCByIndex(0); // Get temperature data from sensors
  Blynk.virtualWrite(V0, temperatureC);  // Send temperature to Blynk on Virtual Pin V0
}

void showDataToDisplay() {
    float temperatureNow = temperatureSensor.getTempCByIndex(0);
    float tdsNow = readTds();

    char temperatureStr[10];
    char tdsStr[10];

    // Convert Float to String
    dtostrf(temperatureNow, 4, 1, temperatureStr);
    dtostrf(tdsNow, 4, 1, tdsStr);

    // Clear display buffer
    oledDisplay.clearBuffer();

    // Draw weather image and temperature value
    oledDisplay.drawXBMP(10, 10, 16, 16, image_weather_temperature_bits);
    oledDisplay.setFont(u8g2_font_profont17_tr);
    oledDisplay.drawStr(35, 23, temperatureStr);

    // Draw weather image and tds value
    oledDisplay.drawXBMP(10, 39, 11, 16, image_weather_humidity_bits);
    oledDisplay.drawStr(35, 53, tdsStr);

    // Send to display
    oledDisplay.sendBuffer();
    delay(150);
}
void setup(void) {
  Serial.begin(115200);
  temperatureSensor.begin();

  // OLED Display sensor init
  oledDisplay.begin();
  oledDisplay.setColorIndex(1);
  oledDisplay.setFontMode(1);
  oledDisplay.setBitmapMode(1);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Send temperature data to Blynk every 1s (1000ms)
  timer.setInterval(1000L, sendTempDataTimer);
  // Read TDS value every 1s (1000ms)
  timer.setInterval(1000L, readTds);
}

void loop(void) {
  readTdsAnalogSensor();
  showDataToDisplay();
  Blynk.run();
  timer.run();
}
