const int sensorPin = A1;
float pOut = 0;

void setup() {
  pinMode(sensorPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  int analogPh = analogRead(sensorPin); // Mengambil nilai ADC (0-1023)
  Serial.print("ADC Ph Sensor: ");
  Serial.println(analogPh);

  float voltagePh = analogPh * (5.0 / 1023.0); // Convert nilai ADC menjadi nilai voltage
  Serial.print("Voltage PH: ");
  Serial.println(voltagePh, 2);

  // Linear regression equation
  // pH = 142 * Voltage - 466.01
  pOut = 142 * voltagePh - 466.01;

  Serial.print("pH Value: ");
  Serial.println(pOut, 2);

  delay(6000);  // Delay 1 second
}
