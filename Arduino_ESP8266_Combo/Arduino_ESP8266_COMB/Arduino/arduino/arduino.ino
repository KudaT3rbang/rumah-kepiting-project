#include <arduino-timer.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Pin Definitions
#define ONE_WIRE_BUS 2   // DS18B20 temperature sensor connected to pin D2
#define TdsSensorPin A2  // TDS sensor connected to pin A2
#define DO_PIN A0        // Dissolved Oxygen sensor connected to pin A0

// Constants
#define VREF 5.0        // Voltage reference for DO and TDS (volts)
#define ADC_RES 1024.0  // ADC resolution
#define SCOUNT 30       // Number of samples for TDS
#define TWO_POINT_CALIBRATION 0
#define CAL1_V (600.0)  // Single-point calibration voltage for DO (millivolts)
#define CAL1_T (25.0)   // Calibration temperature for DO
#define CAL2_V (1300.0) // Two-point calibration voltage for DO (millivolts)
#define CAL2_T (15.0)   // Calibration temperature for DO

// DO Table (in mg/L) for different temperatures
const uint16_t DO_Table[41] = {
    14460, 14220, 13820, 13440, 13090, 12740, 12420, 12110, 11810, 11530,
    11260, 11010, 10770, 10530, 10300, 10080, 9860, 9660, 9460, 9270,
    9080, 8900, 8730, 8570, 8410, 8250, 8110, 7960, 7820, 7690,
    7560, 7430, 7300, 7180, 7070, 6950, 6840, 6730, 6630, 6530, 6410};

// Globals
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);
auto timerReading = timer_create_default();

float temperatureC = 25.0;  // Default temperature if not read
uint16_t analogBuffer[SCOUNT];
uint8_t analogBufferIndex = 0;
float tdsValue = 0.0;
float averageVoltage = 0.0;
float ADC_Raw = 0.0, ADC_Voltage = 0.0;
float DO = 0.0;

// Function to request temperature
void requestTemperature() {
  tempSensor.requestTemperatures();
  temperatureC = tempSensor.getTempCByIndex(0);
}

// Median filter for TDS
uint16_t getMedianNum(uint16_t bArray[], uint8_t iFilterLen) {
  uint16_t bTab[iFilterLen];
  for (uint8_t i = 0; i < iFilterLen; i++) bTab[i] = bArray[i];
  for (uint8_t j = 0; j < iFilterLen - 1; j++) {
    for (uint8_t i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        uint16_t temp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = temp;
      }
    }
  }
  return (iFilterLen % 2 == 0) ? (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2 : bTab[iFilterLen / 2];
}

// Read TDS values
void readTds() {
  static unsigned long analogSampleTimepoint = millis();
  if (millis() - analogSampleTimepoint > 40U) {
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);
    analogBufferIndex++;
    if (analogBufferIndex == SCOUNT) analogBufferIndex = 0;
  }
}

// Calculate TDS
float calculateTDS() {
  requestTemperature();
  uint16_t analogBufferTemp[SCOUNT];
  for (uint8_t i = 0; i < SCOUNT; i++) analogBufferTemp[i] = analogBuffer[i];
  averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * VREF / ADC_RES;
  float compensationCoefficient = 1.0 + 0.02 * (temperatureC - 25.0);
  float compensationVoltage = averageVoltage / compensationCoefficient;
  tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
              - 255.86 * compensationVoltage * compensationVoltage
              + 857.39 * compensationVoltage) * 0.5;
  return tdsValue;
}

// Calculate DO
float calculateDO(float voltage_mv, float temperature_c) {
#if TWO_POINT_CALIBRATION == 0
  float V_saturation = CAL1_V + 35.0 * (temperature_c - CAL1_T);
  return (voltage_mv * DO_Table[(int)temperature_c] / V_saturation);
#else
  float V_saturation = ((temperature_c - CAL2_T) * (CAL1_V - CAL2_V) / (CAL1_T - CAL2_T)) + CAL2_V;
  return (voltage_mv * DO_Table[(int)temperature_c] / V_saturation);
#endif
}

// Print all sensor data
bool printSensorData(void*) {
  float tds = calculateTDS();
  ADC_Raw = analogRead(DO_PIN);
  ADC_Voltage = (ADC_Raw * VREF) / ADC_RES;  // Convert raw ADC value to voltage
  DO = calculateDO(ADC_Voltage * 1000, temperatureC); // Convert V to mV for calculation

  String sendData = "";
  sendData += temperatureC;  // Add temperature
  sendData += ",";           // Add separator
  sendData += tds;           // Add TDS value
  sendData += ",";           // Add separator
  sendData += DO / 1000.0;   // Add DO value in mg/L

  Serial.println(sendData);
  return true;  // Repeat the task
}

void setup() {
  Serial.begin(115200);
  tempSensor.begin();
  timerReading.every(2000, printSensorData); // Print every 2 seconds
}

void loop() {
  readTds();
  timerReading.tick();
}
