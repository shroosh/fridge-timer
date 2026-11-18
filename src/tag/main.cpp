#include <Arduino.h>

const int LDR_PIN = 34;
const int LED_PIN = 21;
const int lightTreshold = 2000;

void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  int lightLevel = analogReadMilliVolts(LDR_PIN);

  if (lightLevel <  lightTreshold) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }


  #ifdef DEBUG
  Serial.printf("%d mV\n", lightLevel);
  #endif

  delay(500);
}