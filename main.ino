#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Keypad.h>

// =============================
// DISPLAY
// =============================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire1, -1);

// =============================
// KEYPAD
// =============================
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {32, 33, 25, 26}; // Rows R1-R4
byte colPins[COLS] = {27, 14, 12, 13}; // Columns C1-C4

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// =============================
// USER DATA & STATE MANAGEMENT
// =============================
String weightInput = "";
String feetInput = "";
String inchesInput = "";
float weight_lb = 0;
int heightFeet = 0;
int heightInches = 0;

// ✅ Use an enum for a clear state machine
enum AppState {
  INPUT_WEIGHT,
  INPUT_HEIGHT_FEET,
  INPUT_HEIGHT_INCHES,
  RUNNING
};
AppState currentState = INPUT_WEIGHT;

// =============================
// CONFIGURATION
// =============================
Adafruit_MPU6050 mpu; 
WebServer server(80);

const char* ssid = "Iphkne 15";
const char* password = "esp32tes";

int stepCount = 0;
const float CALORIES_PER_STEP = 0.04;

unsigned long lastStepTime = 0;
unsigned long lastDisplayUpdateTime = 0; // ✅ For non-blocking display updates

// =============================
// HTML PAGE TEMPLATE
// =============================
String getPage() {
  float calories = stepCount * CALORIES_PER_STEP;
  
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>ESP32 Step Counter</title>
    <style>
      body { font-family: Arial, sans-serif; text-align: center; background: #f0f2f5; color: #333; margin-top: 60px; }
      h1 { color: #2c3e50; }
      .count { font-size: 3em; color: #27ae60; margin: 20px; }
      .calories { font-size: 1.5em; color: #e67e22; margin: 10px; }
      .label { font-size: 0.8em; color: #7f8c8d; text-transform: uppercase; letter-spacing: 2px; }
      button { background-color: #3498db; border: none; color: white; padding: 10px 20px; font-size: 1em; margin: 10px; border-radius: 10px; cursor: pointer; }
      button:hover { background-color: #2980b9; }
      .reset-btn { background-color: #e74c3c; }
      .reset-btn:hover { background-color: #c0392b; }
    </style>
    <meta http-equiv="refresh" content="2"> <!-- Refresh page every 2s -->
  </head>
  <body>
    <h1>ESP32 Step Counter</h1>
    <div class="label">Steps</div>
    <div class="count">)rawliteral";
  html += String(stepCount);
  html += R"rawliteral(</div>
    <div class="label">Calories Burned</div>
    <div class="calories">)rawliteral";
  html += String(calories, 1); // 1 decimal place
  html += R"rawliteral( cal</div>
    <form action="/reset">
      <button type="submit" class="reset-btn">Reset</button>
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
  lastStepTime = 0;
  server.sendHeader("Location", "/", true);
  server.send(303, "text/plain", "Redirecting to home...");
}

// =============================
// SETUP
// =============================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(21, 22); 
  Wire1.begin(19, 18);
  
  // OLED setup
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found on I2C Bus 1 (SDA=19, SCL=18)! Check wiring.");
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // MPU6050 setup
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 on I2C Bus 0 (SDA=21, SCL=22)! Check wiring.");
    while (1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("MPU6050 initialized");

  // Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP: ");
  Serial.println(WiFi.localIP());

  // Web server routes
  server.on("/", handleRoot);
  server.on("/reset", handleReset);
  server.begin();
  Serial.println("Server started!");

  // Start with the first input screen
  displayWeightScreen();
}

// =============================
// STEP DETECTION
// =============================
double getAccelerationMagnitude(sensors_event_t a) {
  return sqrt(a.acceleration.x*a.acceleration.x +
              a.acceleration.y*a.acceleration.y +
              a.acceleration.z*a.acceleration.z);
}

bool stepDetected(double magnitude) {
  static double filteredMag = 0;
  static double prevMag = 0;
  const float STEP_THRESHOLD_HIGH = 11.5; // Slightly adjusted for more robust detection
  const float DEBOUNCE_TIME = 300; // ms to wait between steps
  static unsigned long lastPeakTime = 0;

  float alpha = 0.2; // Low-pass filter
  filteredMag = alpha * magnitude + (1 - alpha) * filteredMag;

  unsigned long now = millis();
  bool stepFound = false;

  // Peak detection logic
  if (filteredMag > prevMag && filteredMag > STEP_THRESHOLD_HIGH && now - lastPeakTime > DEBOUNCE_TIME) {
    lastPeakTime = now;
    lastStepTime = now; // Also update the global step time
    stepFound = true;
  }
  
  prevMag = filteredMag;
  return stepFound;
}

// =============================
// DISPLAY FUNCTIONS
// =============================
void displayWeightScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Enter Weight (lb):");
  display.setTextSize(2);
  display.setCursor(0,20);
  display.println(weightInput);
  display.setCursor(0, 40);
  display.setTextSize(1);
  display.println("'*' = DEL, '#' = OK");
  display.display();
}

void displayHeightFeetScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Enter Height (feet):");
  display.setTextSize(2);
  display.setCursor(0,20);
  display.println(feetInput);
  display.setCursor(0, 40);
  display.setTextSize(1);
  display.println("'*' = DEL, '#' = OK");
  display.display();
}

void displayHeightInchesScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Enter Height (inches):");
  display.setTextSize(2);
  display.setCursor(0,20);
  display.println(inchesInput);
  display.setCursor(0, 40);
  display.setTextSize(1);
  display.println("'*' = DEL, '#' = OK");
  display.display();
}

void displayResults() {
  float totalInches = heightFeet * 12 + heightInches;
  float height_m = totalInches * 0.0254;
  float weight_kg = weight_lb * 0.453592;
  float bmi = (height_m > 0) ? (weight_kg / (height_m * height_m)) : 0;

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Data Saved! Starting...");
  display.print("Weight: "); display.print(weight_lb); display.println(" lb");
  display.print("Height: "); display.print(heightFeet); display.print("'"); display.print(heightInches); display.println("\"");
  display.print("BMI: "); display.println(bmi,1);
  display.display();
}

// ✅ New function to show the main step counter screen
void displayStepCounterScreen() {
  float calories = stepCount * CALORIES_PER_STEP;
  
  display.clearDisplay();
  
  // Display Step Count
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Steps:");
  display.setTextSize(3);
  display.setCursor(15, 12);
  display.print(stepCount);

  // Display Calories
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("Cals: ");
  display.print(calories, 1);

  // Display IP Address
  display.setCursor(0, 56);
  display.print("IP: ");
  display.print(WiFi.localIP());

  display.display();
}


// =============================
// HELPER FUNCTIONS
// =============================

// ✅ New function to handle all keypad logic
void handleKeypadInput() {
  char key = keypad.getKey();
  if (!key) return; // No key pressed

  switch(currentState) {
    case INPUT_WEIGHT:
      if (key >= '0' && key <= '9') weightInput += key;
      else if (key == '*') { if (weightInput.length() > 0) weightInput.remove(weightInput.length() - 1); }
      else if (key == '#') {
        weight_lb = weightInput.toFloat();
        currentState = INPUT_HEIGHT_FEET;
        displayHeightFeetScreen();
        return; // Return early to avoid double-drawing
      }
      displayWeightScreen();
      break;

    case INPUT_HEIGHT_FEET:
      if (key >= '0' && key <= '9') feetInput += key;
      else if (key == '*') { if (feetInput.length() > 0) feetInput.remove(feetInput.length() - 1); }
      else if (key == '#') {
        heightFeet = feetInput.toInt();
        currentState = INPUT_HEIGHT_INCHES;
        displayHeightInchesScreen();
        return;
      }
      displayHeightFeetScreen();
      break;

    case INPUT_HEIGHT_INCHES:
      if (key >= '0' && key <= '9') inchesInput += key;
      else if (key == '*') { if (inchesInput.length() > 0) inchesInput.remove(inchesInput.length() - 1); }
      else if (key == '#') {
        heightInches = inchesInput.toInt();
        currentState = RUNNING;
        displayResults(); // Show summary screen
        delay(3000);      // Pause for 3 seconds to let user read it
      } else {
        displayHeightInchesScreen();
      }
      break;

    case RUNNING:
      // Keypad could be used for other functions here in the future, like reset
      break;
  }
}

void handleStepCounting() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  double magnitude = getAccelerationMagnitude(a);

  if (stepDetected(magnitude)) {
    stepCount++;
    Serial.print("Step detected! Total: ");
    Serial.print(stepCount);
    Serial.print(" | Calories: ");
    Serial.println(stepCount * CALORIES_PER_STEP);
  }

  // ✅ Update the display periodically without blocking
  if (millis() - lastDisplayUpdateTime > 500) { // Update every 500ms
    displayStepCounterScreen();
    lastDisplayUpdateTime = millis();
  }
}

// =============================
// MAIN LOOP
// =============================
void loop() {
  server.handleClient(); // Always handle web server requests

  // ✅ A clean loop that delegates based on the current state
  switch(currentState) {
    case INPUT_WEIGHT:
    case INPUT_HEIGHT_FEET:
    case INPUT_HEIGHT_INCHES:
      handleKeypadInput();
      break;

    case RUNNING:
      handleStepCounting();
      break;
  }
  
  delay(20); // Small delay to prevent overwhelming the MPU
}