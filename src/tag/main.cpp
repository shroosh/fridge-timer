#include <Arduino.h>

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  // LED on
  delay(500);                      // 1 second
  digitalWrite(LED_BUILTIN, LOW);   // LED off
  delay(500);                      // 1 second
}