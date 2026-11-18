#include <Arduino.h>
#include <LiquidCrystal.h>

// Pins match: RS=13, E=27, D4=33, D5=15, D6=32, D7=14
LiquidCrystal lcd(13, 27, 33, 15, 32, 14);

void setup() {
  // Turn on virtual ground pins BEFORE starting the LCD
  pinMode(25, OUTPUT); digitalWrite(25, LOW); // A1 -> Contrast (V0)
  pinMode(26, OUTPUT); digitalWrite(26, LOW); // A0 -> Write-only (RW)
  pinMode(21, OUTPUT); digitalWrite(21, LOW); // 21 -> Backlight - (K)

  lcd.begin(16, 2);
  lcd.print("nyt toimii vittu!!!!");
}

void loop() {
  lcd.setCursor(0, 1);
  lcd.print("!!!!!!!!!!!!!!!!");
  delay(500);
}