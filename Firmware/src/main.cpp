#include <Arduino.h>

const int steeringPin = A0; 
const int accelPin = A1;

void setup() {
  Serial.begin(115200);
}

void loop() {
// Read analog values (0 to 1023)
  int steeringVal = analogRead(steeringPin);
  int accelVal = analogRead(accelPin);

  // Send data to PC: "Steering,Accel"
  // Example output: "512,0"
  Serial.print(steeringVal);
  Serial.print(",");
  Serial.println(accelVal);

  // Small delay to prevent flooding the Serial buffer
  // 10ms = 100Hz update rate, plenty for testing
  delay(10);
}


