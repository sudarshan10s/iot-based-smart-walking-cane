#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  mpu.initialize();

  if (mpu.testConnection()) {
    Serial.println("MPU6050 Connected");
  } else {
    Serial.println("Connection Failed");
  }
}

void loop() {

  int16_t ax, ay, az;
  mpu.getAcceleration(&ax, &ay, &az);

  Serial.print("AX: "); Serial.print(ax);
  Serial.print(" AY: "); Serial.print(ay);
  Serial.print(" AZ: "); Serial.println(az);

  // Fall detection logic
  if (abs(ax) > 15000 || abs(ay) > 15000 || abs(az) < 5000) {
    Serial.println("🚨 FALL DETECTED");
  } else {
    Serial.println("NORMAL");
  }

  Serial.println("----------------------");
  delay(500);
}
