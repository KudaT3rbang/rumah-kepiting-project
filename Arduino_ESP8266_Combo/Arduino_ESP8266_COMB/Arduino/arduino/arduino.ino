#include <arduino-timer.h>

// Include Temperature Sensors Library
#include <OneWire.h>
#include <DallasTemperature.h>

// Temperature Sensors
#define ONE_WIRE_BUS 2 // Temperature Sensors (DS18B20) wire connected to D2 on Arduino
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

float temperatureC;

// Timer for reading data
auto timerReading = timer_create_default();

// TDS Sensors
#define TdsSensorPin A2 // TDS Sensors wire connected to A2 on arduino
#define VREF 3.3
#define SCOUNT 30

uint16_t analogBuffer[SCOUNT];
uint16_t analogBufferTemp[SCOUNT];
uint8_t analogBufferIndex = 0;
uint8_t copyIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;

void requestTemperature() {
  tempSensor.requestTemperatures();
  temperatureC = tempSensor.getTempCByIndex(0); 
}

uint16_t getMedianNum(uint16_t bArray[], uint8_t iFilterLen) {
  uint16_t bTab[iFilterLen];
  for (uint8_t i = 0; i < iFilterLen; i++) {
    bTab[i] = bArray[i];
  }
  // Sort the array for median calculation
  for (uint8_t j = 0; j < iFilterLen - 1; j++) {
    for (uint8_t i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        uint16_t bTemp = bTab[i];
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
      analogBufferIndex = 0;  // Reset index when buffer is full
    }
  }
}

float calculateTDS() {
  // Copy buffer for processing
  requestTemperature();
  for (uint8_t i = 0; i < SCOUNT; i++) {
    analogBufferTemp[i] = analogBuffer[i];
  }
  averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;

  // Use actual temperature from sensor for compensation
  float compensationCoefficient = 1.0 + 0.02 * (temperatureC - 25.0);
  float compensationVoltage = averageVoltage / compensationCoefficient;

  tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
              - 255.86 * compensationVoltage * compensationVoltage
              + 857.39 * compensationVoltage) * 0.5;
  return tdsValue;
}

bool printTdsAndTemperature(void*) {
  float tds = calculateTDS();  // Calculate TDS value

  // Send Temperature and TDS value to ESP8266 Via Serial
  String sendData = "";
  sendData += temperatureC;
  sendData += ",";
  sendData += tds;
  Serial.println(sendData);
  
  return true;  // Repeat this task
}

void setup(void) {
  Serial.begin(115200);
  tempSensor.begin();
  timerReading.every(2000, printTdsAndTemperature);
}

void loop(void) {
  readTds();
  timerReading.tick();
}