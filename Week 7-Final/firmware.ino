#include <WiFi.h>
#include <IOXhop_FirebaseESP32.h>
#include <TinyGPSPlus.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// --- Firebase & WiFi Configuration ---
#define FIREBASE_HOST "warsurveillance-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "Vr8gSntwSVjJgGZf2Ohfk4fLqFh5RCuYtJuqynBb"
#define WIFI_SSID "vivo"
#define WIFI_PASSWORD "12345678"

// --- Pin Definitions ---
#define TRIG_PIN 5
#define ECHO_PIN 18
#define BUZZER_PIN 19
#define VIBRATOR_PIN 23
#define IR_PIN 4
#define LDR_PIN 12
#define LED_PIN 2
#define BUTTON_PIN 15
#define WATER_PIN 32   // Water Level Sensor Pin

// --- Objects & Globals ---
TinyGPSPlus gps;
#define gpsSerial Serial2
Adafruit_MPU6050 mpu;

const float default_lat = 13.095978;
const float default_lng = 77.590175;

// --- State Tracking & Timers ---
int lastFallState = -1;
int lastButtonState = -1;
String lastLdrState = "";
unsigned long lastGpsUpdate = 0;
unsigned long lastDebugPrint = 0; // Timer for Serial Monitor

void setup() {
  Serial.begin(115200);
  gpsSerial.begin(9600, SERIAL_8N1, 16, 17);

  // Initialize Pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(VIBRATOR_PIN, OUTPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(LDR_PIN, INPUT); 
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(WATER_PIN, INPUT);

  // Initialize WiFi
  Serial.print("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  // Initialize MPU6050 with explicit I2C pins
  Wire.begin(21, 22);
  if (!mpu.begin(0x68, &Wire, 0)) {
    Serial.println("Failed to find MPU6050 chip. Check wiring!");
    while (1) { delay(10); } 
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  
  Serial.println("System Ready. Starting Main Loop...\n");
}

void loop() {
  // 1. Keep Reading GPS
  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
  }

  // 2. Read All Sensors
  long duration, distance;
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = duration * 0.034 / 2;

  int irStatus = digitalRead(IR_PIN);          // LOW = Detected
  int ldrStatus = digitalRead(LDR_PIN);        // HIGH = Dark
  int buttonStatus = digitalRead(BUTTON_PIN);  // LOW = Pressed
  int waterStatus = digitalRead(WATER_PIN);    // HIGH = Water Detected

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float angleY = atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;

  // ==========================================
  // DEBUG PRINT SECTION (Prints every 1 second)
  // ==========================================
  if (millis() - lastDebugPrint > 1000) {
    Serial.println("\n--- Live Sensor Status ---");
    Serial.print("Water Sensor: "); Serial.println(waterStatus == HIGH ? "WET (Detected)!" : "Dry");
    Serial.print("Distance:     "); Serial.print(distance); Serial.println(" cm");
    Serial.print("IR Sensor:    "); Serial.println(irStatus == LOW ? "OBSTACLE DETECTED!" : "Clear");
    Serial.print("LDR (Light):  "); Serial.println(ldrStatus == HIGH ? "DARK (Night)" : "BRIGHT (Day)");
    Serial.print("Button:       "); Serial.println(buttonStatus == LOW ? "PRESSED!" : "Idle");
    Serial.print("MPU6050 Tilt: "); Serial.print(angleY); Serial.println(" degrees");
    Serial.print("GPS Sats:     "); Serial.println(gps.satellites.value());
    Serial.println("--------------------------");
    lastDebugPrint = millis(); 
  }

  // ==========================================
  // ALARMS & SOUND LOGIC (Priority Based)
  // ==========================================
  
  if (waterStatus == HIGH) {
    // Priority 1: Water Detected -> Solid Long Sound + Vibrate
    digitalWrite(VIBRATOR_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    gpsDelay(800);
    digitalWrite(BUZZER_PIN, LOW);
    gpsDelay(200);
  } 
  else if (distance > 0 && distance < 30) {
    // Priority 2: Very Close (<30) -> Fast Sound + Vibrate
    digitalWrite(VIBRATOR_PIN, HIGH);
    playBuzzer(150, 150);
  }
  else if (irStatus == LOW) {
    // Priority 3: IR Detected -> Very Rapid Sound + Vibrate
    digitalWrite(VIBRATOR_PIN, HIGH);
    playBuzzer(50, 50); 
  }
  else if (distance >= 30 && distance < 50) {
    // Priority 4: Approaching (30-50) -> Slow Sound, NO Vibrate
    digitalWrite(VIBRATOR_PIN, LOW);
    playBuzzer(400, 400);
  } 
  else {
    // Nothing detected, turn alarms off
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(VIBRATOR_PIN, LOW);
  }

  // ==========================================
  // FIREBASE UPDATES (State Change & Timers)
  // ==========================================

  // A. Fall Detection (1 or 0)
  int currentFallState = (angleY > 3.0 || angleY < -3.0) ? 1 : 0;
  if (currentFallState != lastFallState) {
    Firebase.setInt("SystemStatus/FallDetected", currentFallState);
    Serial.println(currentFallState == 1 ? ">> FIREBASE UPLOAD: Fall Detected (1)" : ">> FIREBASE UPLOAD: Fall Cleared (0)");
    lastFallState = currentFallState;
  }

  // B. Emergency Button (1 or 0)
  int currentButtonState = (buttonStatus == LOW) ? 1 : 0;
  if (currentButtonState != lastButtonState) {
    Firebase.setInt("SystemStatus/EmergencyButton", currentButtonState);
    Serial.println(currentButtonState == 1 ? ">> FIREBASE UPLOAD: Button Pressed (1)" : ">> FIREBASE UPLOAD: Button Released (0)");
    lastButtonState = currentButtonState;
  }

  // C. LDR Environment ("dark" or "light") & LED Control
  String currentLdrState = (ldrStatus == HIGH) ? "dark" : "light";
  digitalWrite(LED_PIN, (ldrStatus == HIGH) ? HIGH : LOW); // LED ON if dark

  if (currentLdrState != lastLdrState) {
    Firebase.setString("SystemStatus/Environment", currentLdrState);
    Serial.println(">> FIREBASE UPLOAD: Environment is now " + currentLdrState);
    lastLdrState = currentLdrState;
  }

  // D. GPS Location (Updates every 5 seconds)
  if (millis() - lastGpsUpdate > 5000) {
    if (gps.location.isValid()) {
      Firebase.setFloat("SystemStatus/Latitude", gps.location.lat());
      Firebase.setFloat("SystemStatus/Longitude", gps.location.lng());
      Serial.println(">> FIREBASE UPLOAD: Live GPS Location Updated.");
    } else {
      Firebase.setFloat("SystemStatus/Latitude", default_lat);
      Firebase.setFloat("SystemStatus/Longitude", default_lng);
      Serial.println(">> FIREBASE UPLOAD: Default GPS Location Sent (Waiting for Satellite Fix...).");
    }
    lastGpsUpdate = millis();
  }
}

// --- Helper Functions ---

// Delay that prevents losing GPS satellite connection
void gpsDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (gpsSerial.available() > 0) {
      gps.encode(gpsSerial.read());
    }
  } while (millis() - start < ms);
}

// Plays a buzzer pattern safely
void playBuzzer(int onTime, int offTime) {
  digitalWrite(BUZZER_PIN, HIGH);
  gpsDelay(onTime);
  digitalWrite(BUZZER_PIN, LOW);
  gpsDelay(offTime);
}   