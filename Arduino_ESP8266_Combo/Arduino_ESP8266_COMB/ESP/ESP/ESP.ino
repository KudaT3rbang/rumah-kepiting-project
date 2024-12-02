#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

float temperature = 0;
float tds = 0;
float dissolvedOxygen = 0; // Variable for Dissolved Oxygen
String incomingString;

// Replace with your network credentials
const char* ssid = "rumah blimbing_4G";
const char* password = "xinna321p";

// Replace with your server URL
const char* serverUrl = "http://192.168.18.42:3000/data"; // Replace <SERVER_IP> with your server's IP

void setup() {
  Serial.begin(115200);

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Print local IP address
  Serial.println("\nWiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Listen for incoming serial data
  if (Serial.available() > 0) {
    incomingString = Serial.readStringUntil('\n');
    Serial.println("Received data: " + incomingString);

    // Parse the incoming string (e.g., "25.3,120.5,6.8")
    int firstComma = incomingString.indexOf(',');
    int secondComma = incomingString.indexOf(',', firstComma + 1);

    if (firstComma != -1 && secondComma != -1) {
      String tempString = incomingString.substring(0, firstComma);
      String tdsString = incomingString.substring(firstComma + 1, secondComma);
      String doString = incomingString.substring(secondComma + 1);

      // Convert strings to float
      temperature = tempString.toFloat();
      tds = tdsString.toFloat();
      dissolvedOxygen = doString.toFloat();
    }
  }

  // Upload data to the server
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;

    http.begin(client, serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Create JSON payload
    String payload = "{\"temperature\":" + String(temperature) + 
                     ",\"tds\":" + String(tds) + 
                     ",\"dissolvedOxygen\":" + String(dissolvedOxygen) + "}";

    // Send HTTP POST request
    int httpResponseCode = http.POST(payload);

    // Print the response
    if (httpResponseCode > 0) {
      Serial.println("POST Response code: " + String(httpResponseCode));
      String response = http.getString();
      Serial.println("Server response: " + response);
    } else {
      Serial.println("Error sending POST request: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("Wi-Fi not connected");
  }

  delay(20000);
}
