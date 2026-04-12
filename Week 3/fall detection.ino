#define TRIG 5
#define ECHO 18

long duration;
float distance;

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
}

void loop() {

  // Trigger pulse
  digitalWrite(TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  // Read echo
  duration = pulseIn(ECHO, HIGH);
  distance = duration * 0.034 / 2;

  // Print distance
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Alert logic
  if (distance < 30) {
    Serial.println("VERY CLOSE OBSTACLE");
  }
  else if (distance < 80) {
    Serial.println("OBSTACLE AHEAD");
  }
  else {
    Serial.println("SAFE");
  }

  Serial.println("-------------------");
  delay(1000);
}
