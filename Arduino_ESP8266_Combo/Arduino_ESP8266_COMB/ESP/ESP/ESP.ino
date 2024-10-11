#include <ESP8266WiFi.h>

float temperature = 0;
float tds = 0;
String incomingString;

// Replace with your network credentials
char ssid[] = "rumah blimbing_4G";
char password[] = "xinna321p";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

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
  
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop() {
  // Listen for incoming serial data
  if (Serial.available() > 0) {  
    incomingString = Serial.readStringUntil('\n');
    Serial.println(incomingString);
    
    // Parsing incomingString to extract temperature and TDS data
    int commaIndex = incomingString.indexOf(',');

    if (commaIndex != -1) {
      String tempString = incomingString.substring(0, commaIndex);
      String tdsString = incomingString.substring(commaIndex + 1);

      temperature = tempString.toFloat();
      tds = tdsString.toFloat();
    }
  }

  // Listen for incoming clients for the web server
  WiFiClient client = server.available();

  if (client) {
    Serial.println("New Client.");
    String currentLine = "";
    currentTime = millis();
    previousTime = currentTime;

    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        header += c;

        if (c == '\n') {
          if (currentLine.length() == 0) {
            // HTTP headers
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // HTML content displaying the temperature and TDS
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            client.println("<style>html { font-family: Helvetica; text-align: center; margin: 0px auto;}</style></head>");
            client.println("<body><h1>ESP8266 Data Monitoring</h1>");

            // Display the temperature and TDS data
            client.println("<p>Temperature: " + String(temperature) + " &#8451;</p>");
            client.println("<p>TDS: " + String(tds) + " ppm</p>");

            client.println("</body></html>");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    // Clear the header variable
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
  }
  delay(2000);
}
