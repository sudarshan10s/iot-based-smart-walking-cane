#include <WiFi.h>
#include <HTTPClient.h>

// WiFi (Wokwi default)
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ThingSpeak
String apiKey = "5OS1FIG2SQO00X8Q";
const char* server = "http://api.thingspeak.com/update";

// Pins
int trigPin = 5;
int echoPin = 18;
int ldrPin = 13;     // DO pin
int buzzer = 4;
int led = 2;
int button = 15;

long duration;
int distance;

void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ldrPin, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(led, OUTPUT);
  pinMode(button, INPUT_PULLUP);

  // WiFi connect
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected ✅");
}

void loop() {

  // 🔹 ULTRASONIC (Distance)
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;

  // 🔹 LDR (Digital)
  int light = digitalRead(ldrPin); 
  // HIGH = Dark, LOW = Bright

  // 🔹 BUTTON (Emergency)
  int emergency = (digitalRead(button) == LOW) ? 1 : 0;

  // 🔹 Print values
  Serial.println("Distance: " + String(distance));
  Serial.println("Light (0=Bright,1=Dark): " + String(light));
  Serial.println("Emergency: " + String(emergency));

  // 🔴 PoC 3: ALERT LOGIC
  if (distance < 50 || light == HIGH || emergency == 1) {
    digitalWrite(buzzer, HIGH);
    digitalWrite(led, HIGH);
    Serial.println("⚠ ALERT TRIGGERED");
  } else {
    digitalWrite(buzzer, LOW);
    digitalWrite(led, LOW);
  }

  // ☁️ PoC 2: CLOUD (ThingSpeak)
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = String(server) + "?api_key=" + apiKey +
                 "&field1=" + distance +
                 "&field2=" + light +
                 "&field3=" + emergency;

    http.begin(url);
    http.GET();
    http.end();
  }

  Serial.println("-----------------------");

  delay(15000);
}
