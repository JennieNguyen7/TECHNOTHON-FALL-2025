# TECHNOTHON-FALL-2025

This project is an ESP32 based portable fitness tracker that has a step counter, BMI calculator, and real time display. 
The user inputs their weight and height in order to get their BMI calculated and displayed on the OLED display. 
The step and calorie counter are then displayed on a Web app for easy access and readability.

## Hardware Components
ESP32, IMU, OLED Display, 4x4 Numpad, 3-D Printed Case

## Libraries
- Adafruit SSD1306 & GFX  
- Adafruit MPU6050  
- Keypad  
- Arduino WiFi & WebServer  

## Setup
1. Connect hardware (OLED, MPU6050, Keypad) to ESP32.  
2. Install required Arduino libraries.  
3. Update Wi-Fi credentials in the code.  
4. Upload code to the ESP32 and access the web app via the Serial Monitor IP.

## Team
- Jennie Nguyen: Coded the OLED Display and BMI calculations 
- Karthik Gourabathuni: Programmed the IMU and Step calculations
- WD Batteas: Designed a hardware casing to house all components
