#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>

// Pin definition
const int irSensorPin = 12;  // Pin IR sensor terhubung
const int ledPin1 = 16;      // LED 1
const int ledPin2 = 17;      // LED 2
const int espCamTriggerPin = 13;  // Pin untuk memberi sinyal ke ESP32-CAM

// WiFi credentials
const char* ssid = "CEO";
const char* password = "2444666668888888";

// Server ESP32-CAM endpoint
const char* espCamServer = "http://192.168.251.60:8000/upload";

// Web server pada port 8000
WebServer server(8000);

void setup() {
  Serial.begin(115200);

  // Set pin modes
  pinMode(irSensorPin, INPUT);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(espCamTriggerPin, OUTPUT);

  // Initialize pins
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);
  digitalWrite(espCamTriggerPin, LOW);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  // Check if IR sensor detects an object
  if (digitalRead(irSensorPin) == LOW) {
    Serial.println("Object detected!");

    // Turn on the LED lights
    digitalWrite(ledPin1, HIGH);
    digitalWrite(ledPin2, HIGH);

    // Send signal to ESP32-CAM to take a picture
    digitalWrite(espCamTriggerPin, HIGH);
    delay(1000);  // Give time for ESP32-CAM to take picture
    digitalWrite(espCamTriggerPin, LOW);

    // Send HTTP request to ESP32-CAM to trigger image capture (optional)
    if(WiFi.status() == WL_CONNECTED){
      HTTPClient http;
      http.begin(espCamServer);
      int httpResponseCode = http.GET();  // Assuming ESP32-CAM triggers image capture on GET request
      if(httpResponseCode > 0){
        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
      } else {
        Serial.printf("Error code: %d\n", httpResponseCode);
      }
      http.end();
    }

    // Wait for a while before checking for next detection
    delay(5000);  // Delay to prevent immediate retriggering
  } else {
    // If no object is detected, turn off the LEDs
    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, LOW);
  }

  delay(500);  // Small delay for sensor stability
}