#include <WiFi.h>
#include <HTTPClient.h>
#include <WebServer.h>

// Pin definition
const int irSensorPin = 12;       // Pin IR sensor terhubung
const int espCamTriggerPin = 13; // Pin untuk memberi sinyal ke ESP32-CAM
const int ledPin1 = 16;         // LED 1
const int ledPin2 = 17;        // LED 2
const int greenLEDPin = 14;   // Pin untuk LED hijau
const int redLEDPin = 27;    // Pin untuk LED merah
const int buzzerPin = 22;   // Buzzer
const int relayPin = 15;   // Ganti dengan pin relay Anda

// WiFi credentials
const char* ssid = "CEO";
const char* password = "2444666668888888";

// Server ESP32-CAM endpoint
const char* espCamServer = "http://192.168.251.60:8000/upload";

// Web server pada port 8000
WebServer server(8000);

int attemptCount = 0;       // Counting Attempts
const int maxAttempts = 3; // Max Attempts

void setup() {
  Serial.begin(115200);

  // Set pin modes
  pinMode(irSensorPin, INPUT);
  pinMode(espCamTriggerPin, OUTPUT);
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(greenLEDPin, OUTPUT);
  pinMode(redLEDPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  // Initialize pins
  digitalWrite(espCamTriggerPin, LOW);
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);
  digitalWrite(greenLEDPin, LOW);
  digitalWrite(redLEDPin, LOW);
  digitalWrite(buzzerPin, LOW);
  digitalWrite(relayPin, LOW);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); // Menampilkan alamat IP

  // Inisialisasi server
  server.on("/result", HTTP_POST, handleResult);
  server.begin();
}

void loop() {
  server.handleClient();
  // Check if IR sensor detects an object
  if (digitalRead(irSensorPin) == LOW) {
    Serial.println("Object detected!");

    // Turn on the LED lights
    digitalWrite(ledPin1, HIGH);
    digitalWrite(ledPin2, HIGH);

    // Sound the buzzer for 0.3 seconds
    digitalWrite(buzzerPin, HIGH);
    digitalWrite(greenLEDPin, HIGH);
    delay(300);
    digitalWrite(buzzerPin, LOW);
    digitalWrite(greenLEDPin, LOW);

    //delay cam before start capturing
    delay(1000);
    
    // Send signal to ESP32-CAM to take a picture after 1 second
    digitalWrite(espCamTriggerPin, HIGH);
    delay(1000);  // Wait 1 second before capturing
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

void handleResult() {
  if (server.hasArg("plain")) {
    String result = server.arg("plain");
    Serial.println("Results Received: "+ result);

    if (result == "Identik") {
      // Hasil perbandingan identik, reset percobaan
      attemptCount = 0;
      // Hasil perbandingan identik
      digitalWrite(relayPin, HIGH);
      digitalWrite(greenLEDPin, HIGH);
      tone(buzzerPin, 1000, 1000); // Bunyi buzzer selama 1 detik
      delay(5000); //relay dan led hijau akan tetap menyala selama 5 detik
      digitalWrite(greenLEDPin, LOW);
      digitalWrite(relayPin, LOW);
    }else if (result == "Tidak Identik"){
      // Hasil perbandingan tidak identik
      attemptCount++;
      if (attemptCount <= maxAttempts) {
        // LED merah menyala dan buzzer berbunyi setiap percobaan
        digitalWrite(redLEDPin, HIGH);
        for (int i = 0; i < 4; i++) {
          tone(buzzerPin, 1000);
          delay(250);
          noTone(buzzerPin);
          delay(250);
        }
        digitalWrite(redLEDPin, LOW);
      }
      if (attemptCount == maxAttempts) {
        // Percobaan ketiga
        for (int i = 0; i < 10; i++) { // LED merah berkedip
          digitalWrite(redLEDPin, HIGH);
          tone(buzzerPin, 1000);
          delay(500);
          digitalWrite(redLEDPin, LOW);
          noTone(buzzerPin);
          delay(500);
        }
        delay(5000); // Tunggu 5 detik sebelum memulai percobaan lagi
        attemptCount = 0; // Reset percobaan setelah 5 detik
      }
    }else if (result == "Tidak Terdaftar"){
      // Hasil perbandingan tidak identik
      attemptCount++;
      if (attemptCount <= maxAttempts) {
        // LED merah menyala dan buzzer berbunyi setiap percobaan
        digitalWrite(redLEDPin, HIGH);
        for (int i = 0; i < 4; i++) {
          tone(buzzerPin, 1000);
          delay(250);
          noTone(buzzerPin);
          delay(250);
        }
        digitalWrite(redLEDPin, LOW);
      }
      if (attemptCount == maxAttempts) {
        // Percobaan ketiga
        for (int i = 0; i < 10; i++) { // LED merah berkedip
          digitalWrite(redLEDPin, HIGH);
          tone(buzzerPin, 1000);
          delay(500);
          digitalWrite(redLEDPin, LOW);
          noTone(buzzerPin);
          delay(500);
        }
        delay(5000); // Tunggu 5 detik sebelum memulai percobaan lagi
        attemptCount = 0; // Reset percobaan setelah 5 detik
      }
    }
  }
  server.send(200, "text/plain", "Result received");
}