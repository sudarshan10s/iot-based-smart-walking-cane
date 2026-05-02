#define TRIG 27
#define ECHO 26
#define LDR 34
#define RAIN 25
#define BUZZER 14
#define BUTTON 12

long duration;
int distance;

void setup() {
  Serial.begin(115200);

  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  pinMode(RAIN, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(BUTTON, INPUT_PULLUP);
}

void loop() {

  // 📏 ULTRASONIC
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  duration = pulseIn(ECHO, HIGH);
  distance = duration * 0.034 / 2;

  // 🌗 LDR
  int light = analogRead(LDR);

  // 🌧️ RAIN
  int rain = digitalRead(RAIN);

  // 🚨 BUTTON
  int emergency = digitalRead(BUTTON);

  Serial.println("------------ STATUS ------------");
  Serial.print("Distance: "); Serial.println(distance);
  Serial.print("Light: "); Serial.println(light);
  Serial.print("Rain: "); Serial.println(rain);

  // 🚨 EMERGENCY
  if (emergency == LOW) {
    Serial.println("🚨 EMERGENCY ALERT! PLEASE HELP REQUIRED!");

    digitalWrite(BUZZER, HIGH); // continuous
    delay(200);
    return;
  }

  // 📏 OBSTACLE
  if (distance < 50 && distance > 0) {
    Serial.println("⚠ WARNING: Obstacle detected! Please move slightly to the side.");

    for (int i = 0; i < 3; i++) {
      digitalWrite(BUZZER, HIGH);
      delay(100);
      digitalWrite(BUZZER, LOW);
      delay(100);
    }
  }

  // 🌙 DARK
  else if (light < 1000) {
    Serial.println("🌙 WARNING: Low light detected! Please proceed carefully.");

    digitalWrite(BUZZER, HIGH);
    delay(300);
    digitalWrite(BUZZER, LOW);
    delay(300);
  }

  // 🌧️ RAIN
  else if (rain == LOW) {
    Serial.println("🌧 WARNING: Water detected! Surface may be slippery.");

    digitalWrite(BUZZER, HIGH);
    delay(150);
    digitalWrite(BUZZER, LOW);
    delay(150);

    digitalWrite(BUZZER, HIGH);
    delay(150);
    digitalWrite(BUZZER, LOW);
    delay(500);
  }

  // ✅ SAFE
  else {
    Serial.println("✅ SAFE: No obstacles or hazards detected.");
    digitalWrite(BUZZER, LOW);
  }

  delay(300);
}
