#include <Arduino.h>
#include <LiquidCrystal.h>

#include "symbol.hpp"

// Enum for different UI states, which tell loop what code to execute
enum RenderingState {
  START_ANIMATION,  // Plays brief animation upon starting the device
  PASSIVE,          // Shows most relevant tag info
  TAG_EDITING       // UI for selecting and changing tag countdowns
};

// Declare UI state variable
RenderingState current_rendering_state;
uint32_t state_start_ms;  // Tracks start time of current rendering state

const uint8_t B_topleft[8] = {
  0b00000, 
  0b11111, 
  0b11111, 
  0b11000,
  0b11000,
  0b11000,
  0b11000,
  0b11111
};

const uint8_t B_topright[8] = {
  0b00000, 
  0b10000, 
  0b11000, 
  0b11100,
  0b01100,
  0b01100,
  0b11100,
  0b11000
};

const uint8_t B_bottomleft[8] = {
  0b11111, 
  0b11000, 
  0b11000, 
  0b11000,
  0b11000,
  0b11000,
  0b11111,
  0b00000
};

const uint8_t B_bottomright[8] = {
  0b11100, 
  0b01110, 
  0b00110, 
  0b00110,
  0b00110,
  0b01110,
  0b11100,
  0b00000
};

Symbol B_parts[4] = {Symbol(B_topleft), Symbol(B_topright), Symbol(B_bottomleft), Symbol(B_bottomright)};
uint8_t B_CGRAM_slots[4] = {0, 1, 2, 3};


// Pins match: RS=13, E=27, D4=33, D5=15, D6=32, D7=14
LiquidCrystal lcd(13, 27, 33, 15, 32, 14);

void set_rendering_state(RenderingState new_state) {
  current_rendering_state = new_state;
  state_start_ms = millis();

  lcd.clear();
}

void setup() {
  // Turn on virtual ground pins BEFORE starting the LCD
  pinMode(25, OUTPUT);
  digitalWrite(25, LOW);  // A1 -> Contrast (V0)
  pinMode(26, OUTPUT);
  digitalWrite(26, LOW);  // A0 -> Write-only (RW)
  pinMode(21, OUTPUT);
  digitalWrite(21, LOW);  // 21 -> Backlight - (K)

  lcd.begin(16, 2);

  // Fill CGRAM with B parts
  for (uint8_t i = 0; i<4; i++) {
    lcd.createChar(B_CGRAM_slots[i], B_parts[i].get_data());
  }

  // Begin start animation
  set_rendering_state(START_ANIMATION);
}

void loop() {
  // other logic goes here
  //
  //

  // Begin rendering phase
  uint32_t state_elapsed_ms = millis() - state_start_ms;

  switch (current_rendering_state) {
    case START_ANIMATION:

      lcd.setCursor(0, 0);
      lcd.write(B_CGRAM_slots[0]);
      lcd.write(B_CGRAM_slots[1]);
      lcd.setCursor(0, 1);
      lcd.write(B_CGRAM_slots[2]);
      lcd.write(B_CGRAM_slots[3]);
      lcd.print("EST");
      lcd.setCursor(5, 0);
      lcd.write(B_CGRAM_slots[0]);
      lcd.write(B_CGRAM_slots[1]);
      lcd.setCursor(5, 1);
      lcd.write(B_CGRAM_slots[2]);
      lcd.write(B_CGRAM_slots[3]);
      lcd.print("EFORE");
      lcd.setCursor(12, 0);
      lcd.write(B_CGRAM_slots[0]);
      lcd.write(B_CGRAM_slots[1]);
      lcd.setCursor(12, 1);
      lcd.write(B_CGRAM_slots[2]);
      lcd.write(B_CGRAM_slots[3]);
      lcd.print("RO");

      break;
    case PASSIVE:
      // TODO: passive visuals
      break;
    case TAG_EDITING:
      // TODO: tag editing visuals
      break;
  }
}