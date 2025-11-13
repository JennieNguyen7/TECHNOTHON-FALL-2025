#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
// =============================
// CONFIGURATION
// =============================
Adafruit_MPU6050 mpu;
WebServer server(80);

const char* ssid = "TAMU_IoT";
const char* password = "";

int stepCount = 0;  // Replace this with real IMU-based step logic later

// Adaptive threshold variables
const int WINDOW_SIZE = 50; // 1 sec @ 50Hz
float buffer[WINDOW_SIZE];
int bufIndex = 0;
bool bufferFull = false;
float k = 1.1;
unsigned long lastStepTime = 0;

// =============================
// HTML PAGE TEMPLATE
// =============================
String getPage() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>ESP32 Step Counter</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        text-align: center;
        background: #f0f2f5;
        color: #333;
        margin-top: 60px;
      }
      h1 {
        color: #2c3e50;
      }
      .count {
        font-size: 3em;
        color: #27ae60;
        margin: 20px;
      }
      button {
        background-color: #3498db;
        border: none;
        color: white;
        padding: 10px 20px;
        font-size: 1em;
        margin: 10px;
        border-radius: 10px;
        cursor: pointer;
      }
      button:hover {
        background-color: #2980b9;
      }
    </style>
    <meta http-equiv="refresh" content="2"> <!-- Refresh page every 2s -->
  </head>
  <body>
    <h1>ESP32 Step Counter</h1>
    <div class="count">)rawliteral";
  html += String(stepCount);
  html += R"rawliteral(</div>
    <form action="/reset">
      <button type="submit" style="background-color:#e74c3c;">Reset</button>
    </form>
    
  </body>
  </html>
  )rawliteral";
  return html;
}

// =============================
// ROUTES
// =============================
void handleRoot() {
  server.send(200, "text/html", getPage());
}



void handleReset() {
  stepCount = 0;
  bufIndex = 0;
  bufferFull = false;
  lastStepTime = 0;
  for (int i = 0; i < WINDOW_SIZE; i++) buffer[i] = 0;

  // Send redirect header (HTTP 303 means "see other")
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "Redirecting to home...");
}




// =============================
// SETUP + LOOP
// =============================
void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  // Define routes
  server.on("/", handleRoot);

  server.on("/reset", handleReset);

  server.begin();
  Serial.println("Server started!");

  // initialize mpu
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("");


}

/**
double getAccelerationMagnitude(sensors_event_t a){
  double mag = (a.acceleration.x*a.acceleration.x)+(a.acceleration.y*a.acceleration.y)+(a.acceleration.z*a.acceleration.z);
  mag = sqrt(mag);
  return mag;
}

bool stepDetected(double magnitude) {
  buffer[bufIndex++] = magnitude;
  if (bufIndex >= WINDOW_SIZE) { bufIndex = 0; bufferFull = true; }

  if (!bufferFull) return false;

  float mean = 0, stddev = 0;
  for (int i = 0; i < WINDOW_SIZE; i++) mean += buffer[i];
  mean /= WINDOW_SIZE;
  for (int i = 0; i < WINDOW_SIZE; i++) stddev += pow(buffer[i] - mean, 2);
  stddev = sqrt(stddev / WINDOW_SIZE);

  float threshold = mean + k * stddev;
  unsigned long now = millis();

  if (magnitude > threshold && now - lastStepTime > 200) {
    lastStepTime = now;
    return true;
  }
  return false;
}*/
double getAccelerationMagnitude(sensors_event_t a) {
  // Keep total magnitude (including gravity)
  return sqrt(
    a.acceleration.x * a.acceleration.x +
    a.acceleration.y * a.acceleration.y +
    a.acceleration.z * a.acceleration.z
  );
}

bool stepDetected(double magnitude) {
  static double filteredMag = 0;
  static double prevMag = 0;
  static double prevPrevMag = 0;
  static bool wasAboveThreshold = false;
  
  // Smooth the signal
  float alpha = 0.2;
  filteredMag = alpha * magnitude + (1 - alpha) * filteredMag;
  
  // Fixed thresholds work better than adaptive for steps
  const float STEP_THRESHOLD_HIGH = 11.0;  // Peak must exceed this
  const float STEP_THRESHOLD_LOW = 9.5;    // Must drop below this before next peak
  
  unsigned long now = millis();
  bool stepFound = false;
  
  // Detect peak: current value is highest, above threshold, and we crossed low threshold since last peak
  if (filteredMag > STEP_THRESHOLD_HIGH &&
      filteredMag > prevMag && 
      prevMag > prevPrevMag &&
      !wasAboveThreshold &&
      now - lastStepTime > 250) {
    
    wasAboveThreshold = true;
    lastStepTime = now;
    stepFound = true;
  }
  
  // Reset when we drop below low threshold
  if (filteredMag < STEP_THRESHOLD_LOW) {
    wasAboveThreshold = false;
  }
  
  prevPrevMag = prevMag;
  prevMag = filteredMag;
  
  return stepFound;
}

void loop() {
  server.handleClient();

  
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  double magnitude = getAccelerationMagnitude(a);

  if (stepDetected(magnitude)) stepCount++; 
  delay(20);
}