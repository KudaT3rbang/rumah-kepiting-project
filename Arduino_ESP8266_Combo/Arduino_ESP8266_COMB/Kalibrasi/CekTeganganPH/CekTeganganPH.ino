int pH_Value;
float Voltage;

void setup() {
  Serial.begin(9600);
  delay(2000);
}

void loop() {
  pH_Value = analogRead(A0);
  Voltage = pH_Value * (3.3 / 1023.0); // 5 For arduino, 3.3 for ESP8266
  Serial.println(Voltage);
  delay(1000);
}