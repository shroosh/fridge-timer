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

const uint8_t test_symbol_data[8] = {0b00000, 0b01010, 0b01010, 0b00000,
                                     0b10001, 0b01110, 0b00000, 0b00000};

Symbol test_symbol(test_symbol_data);

OpacitySymbol test_opacity_symbol = OpacitySymbol();
uint8_t test_buffer[8] = {0};
uint8_t opacity_counter = 0;
uint8_t opacity = 0;
const uint8_t CGRAM_test_slot = 1;

// Pins match: RS=13, E=27, D4=33, D5=15, D6=32, D7=14
LiquidCrystal lcd(13, 27, 33, 15, 32, 14);

void setup() {
  // Turn on virtual ground pins BEFORE starting the LCD
  pinMode(25, OUTPUT);
  digitalWrite(25, LOW);  // A1 -> Contrast (V0)
  pinMode(26, OUTPUT);
  digitalWrite(26, LOW);  // A0 -> Write-only (RW)
  pinMode(21, OUTPUT);
  digitalWrite(21, LOW);  // 21 -> Backlight - (K)

  lcd.begin(16, 2);

  lcd.createChar(0, test_symbol.get_data());  // Make custom symbol

  lcd.setCursor(0, 0);
  for (uint8_t i = 0; i < 16; i++) {
    lcd.write((uint8_t)CGRAM_test_slot);
  }

  lcd.setCursor(0, 1);
  for (uint8_t i = 0; i < 16; i++) {
    lcd.write((uint8_t)CGRAM_test_slot);
  }

  // Begin start animation
  current_rendering_state = START_ANIMATION;
}

void loop() {
  // other logic goes here
  //
  //

  // Begin rendering phase
  switch (current_rendering_state) {
    case START_ANIMATION:
      for (uint8_t x = 0; x < 5; x++) {
        for (uint8_t y = 0; y < 8; y++) {
          test_opacity_symbol.set_pixel(x, y, (opacity + y * 12) % 101);
        }
      }

      opacity_counter++;
      if (opacity_counter % 10 == 0) {
        opacity = (opacity + 1) % 101;
      }

      test_opacity_symbol.write_data(test_buffer);
      lcd.createChar(CGRAM_test_slot, test_buffer);

      break;
    case PASSIVE:
      // TODO: passive visuals
      break;
    case TAG_EDITING:
      // TODO: tag editing visuals
      break;
  }
}