// #include <Arduino.h>

// void setup() {
//   Serial.begin(115200);
// }

// float steering = 0;
// float throttle = 0;
// float brake = 0;

// char buffer [64];
// int bufferIndex = 0;


// void loop() {

//   // Send data to PC: "Steering,Accel"
//   // Example output: "512,0"
//   while(Serial.available()) {
//     char c = Serial.read();

//       if (c == '\n') {
//         buffer[bufferIndex] = '\0';
        
//         if (sscanf(buffer, "%f,%f,%f", &steering, &throttle, &brake) == 3) {
//           Serial.print("Steer: ");
//           Serial.print(steering);
//           Serial.print("  Throttle: ");
//           Serial.print(throttle);
//           Serial.print("  Brake: ");
//           Serial.println(brake);
//         }

//         bufferIndex = 0;
//       } else {
//         if (bufferIndex < sizeof(buffer) - 1) {
//           buffer[bufferIndex++] = c;
//         }
//       }

//   }
//   delay(10);
// }
#include <Arduino.h>

#define LED_PIN 2 // Most ESP32 DevKits use GPIO 2

void setup() {
  pinMode(LED_PIN, OUTPUT); // Set the LED pin as output
}

void loop() {
  digitalWrite(LED_PIN, HIGH); // Turn the LED on
  delay(1000);                 // Wait for 1000ms (1 second)
  digitalWrite(LED_PIN, LOW);  // Turn the LED off
  delay(1000);                 // Wait for 1000ms (1 second)
}
