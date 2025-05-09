#include "WiFi.h"
#include "esp_camera.h"
#include "WiFiClient.h"
#include "HTTPClient.h"

// WiFi credentials
const char* ssid = "CEO";
const char* password = "2444666668888888";

// Server endpoint for Python image processing
const char* serverName = "http://192.168.251.38:8000/upload";

// Pin configuration
#define PWDN_GPIO_NUM    32
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM    0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27
//#define FLASH_GPIO         4  // GPIO yang terhubung ke lampu flash
#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM      5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

// Pin to receive signal from Wemos Lolin32
const int triggerPin = 13;

void setup() {
  Serial.begin(115200);
  
  // Configure trigger pin
  pinMode(triggerPin, INPUT);

  //pinMode(FLASH_GPIO, OUTPUT);  // Set GPIO 4 sebagai output
  //digitalWrite(FLASH_GPIO, LOW);  // Mulai dengan flash dalam keadaan mati

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP()); // Menampilkan alamat IP
  
  // Camera configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
}

void loop() {
  // Check if Wemos Lolin32 sent a trigger signal
  if (digitalRead(triggerPin) == HIGH) {
    Serial.println("Capture signal received, taking a picture...");

    // Menyalakan flash sebelum mengambil gambar
    //digitalWrite(FLASH_GPIO, HIGH);  

    // Capture image
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }

    // Matikan flash setelah mengambil gambar
    //digitalWrite(FLASH_GPIO, LOW);

    // Send image to server
    if(WiFi.status() == WL_CONNECTED){
      HTTPClient http;
      http.begin(serverName);
      http.addHeader("Content-Type", "image/jpeg");

      int httpResponseCode = http.POST(fb->buf, fb->len);
      if(httpResponseCode > 0){
        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
      } else {
        Serial.printf("Error code: %d\n", httpResponseCode);
      }
      http.end();
    }

    // Return the frame buffer
    esp_camera_fb_return(fb);
  }

  delay(500);  // Small delay to avoid excessive checking
}