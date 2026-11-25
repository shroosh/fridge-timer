#define DEBUG 0

#include <Arduino.h>
#include <LiquidCrystal.h>

#include "../include/connection.hpp"
#include "symbol.hpp"
#include "B_anim_path.cpp"

// Enum for different UI states, which tell loop what code to execute
enum RenderingState {
  START_ANIMATION,  // Plays brief animation upon starting the device
  PASSIVE,          // Shows most relevant tag info
  TAG_EDITING       // UI for selecting and changing tag countdowns
};

// Declare UI state variable
RenderingState current_rendering_state;
uint32_t state_start_ms;  // Tracks start time of current rendering state

// progress for the initial fade animation
size_t B_anim_path_progress = 0;


OpacitySymbol B_parts[4] = {};
uint8_t B_buffers[4][8] = {0};
uint8_t B_CGRAM_slots[4] = {0, 1, 2, 3};


// Pins match: RS=13, E=27, D4=33, D5=15, D6=32, D7=14
LiquidCrystal lcd(13, 27, 33, 15, 32, 14);

void set_rendering_state(RenderingState new_state) {
  current_rendering_state = new_state;
  state_start_ms = millis();

  lcd.clear();
}

void start_animation(uint32_t elapsed_ms) {
  if (elapsed_ms < 3500) { //FADE STAGE

    // Write B characters to screen
    lcd.setCursor(0,0);
    lcd.write(B_CGRAM_slots[0]);
    lcd.write(B_CGRAM_slots[1]);
    lcd.setCursor(0,1);
    lcd.write(B_CGRAM_slots[2]);
    lcd.write(B_CGRAM_slots[3]);
    lcd.setCursor(2,0);
    lcd.write(B_CGRAM_slots[0]);
    lcd.write(B_CGRAM_slots[1]);
    lcd.setCursor(2,1);
    lcd.write(B_CGRAM_slots[2]);
    lcd.write(B_CGRAM_slots[3]);
    lcd.setCursor(4,0);
    lcd.write(B_CGRAM_slots[0]);
    lcd.write(B_CGRAM_slots[1]);
    lcd.setCursor(4,1);
    lcd.write(B_CGRAM_slots[2]);
    lcd.write(B_CGRAM_slots[3]);
    
    // Update symbol buffers, load into CGRAM
    for (uint8_t i = 0; i<4; i++) {
      B_parts[i].write_data(B_buffers[i]); 
      lcd.createChar(B_CGRAM_slots[i], B_buffers[i]);
    }

    constexpr size_t path_len = sizeof(B_anim_path) / sizeof(B_anim_path[0]);
    constexpr int fade_step = 15;  // Opacity drop per pixel (10 = 10-pixel fade, 15 = ~7-pixel fade)
    constexpr uint32_t max_progress = (path_len - 1) * fade_step;

    uint32_t progress = std::min<uint32_t>(elapsed_ms / 3, max_progress);

    constexpr size_t fade_span = (100 + fade_step - 1) / fade_step + 1;
    size_t bound = std::min<size_t>(progress / fade_step + fade_span, path_len);

    for (size_t i = 0; i < bound; i++) {
      uint8_t path_part = B_anim_path[i].first;
      std::pair<uint8_t, uint8_t> coords = B_anim_path[i].second;

      int delta = static_cast<int>(i * fade_step) - static_cast<int>(progress);
      uint8_t opacity = (delta <= 0) ? 100 : (delta >= 100 ? 0 : 100 - delta);

      B_parts[path_part].set_pixel(coords.first, coords.second, opacity);
    }
  } else if (elapsed_ms < 4250) { // "EST" appears
    lcd.clear();
    // Write B characters to screen
    lcd.setCursor(0,0);
    lcd.write(B_CGRAM_slots[0]);
    lcd.write(B_CGRAM_slots[1]);
    lcd.setCursor(0,1);
    lcd.write(B_CGRAM_slots[2]);
    lcd.write(B_CGRAM_slots[3]);
    lcd.print("EST");

    lcd.setCursor(5,0);
    lcd.write(B_CGRAM_slots[0]);
    lcd.write(B_CGRAM_slots[1]);
    lcd.setCursor(5,1);
    lcd.write(B_CGRAM_slots[2]);
    lcd.write(B_CGRAM_slots[3]);
    lcd.setCursor(7,0);
    lcd.write(B_CGRAM_slots[0]);
    lcd.write(B_CGRAM_slots[1]);
    lcd.setCursor(7,1);
    lcd.write(B_CGRAM_slots[2]);
    lcd.write(B_CGRAM_slots[3]);
  } else if (elapsed_ms < 5000) { // "EFORE" appears
    lcd.clear();
    // Write B characters to screen
    lcd.setCursor(0,0);
    lcd.write(B_CGRAM_slots[0]);
    lcd.write(B_CGRAM_slots[1]);
    lcd.setCursor(0,1);
    lcd.write(B_CGRAM_slots[2]);
    lcd.write(B_CGRAM_slots[3]);
    lcd.print("EST");

    lcd.setCursor(5,0);
    lcd.write(B_CGRAM_slots[0]);
    lcd.write(B_CGRAM_slots[1]);
    lcd.setCursor(5,1);
    lcd.write(B_CGRAM_slots[2]);
    lcd.write(B_CGRAM_slots[3]);
    lcd.print("EFORE");

    lcd.setCursor(12,0);
    lcd.write(B_CGRAM_slots[0]);
    lcd.write(B_CGRAM_slots[1]);
    lcd.setCursor(12,1);
    lcd.write(B_CGRAM_slots[2]);
    lcd.write(B_CGRAM_slots[3]);
  } else { // "RO" appears
    lcd.setCursor(14,1);
    lcd.print("RO");
  }
}



// Wifi variables
const uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // MAC


void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  Message msg;
  memcpy(&msg, data, sizeof(msg));
  if (msg.type == Type::CENTRAL_UNIT) {
    switch (msg.state)
    {
    case State::ACTIVATE:
      Message activate1 = {Type::TAG1, State::ACTIVATE};
      Message activate2 = {Type::TAG2, State::ACTIVATE};
      esp_now_send(broadcastAddress, (uint8_t*)&activate1, sizeof(Message));
      esp_now_send(broadcastAddress, (uint8_t*)&activate2, sizeof(Message));
      #ifdef DEBUG
      Serial.println("ACTIVE");
      #endif
      break;
    case State::DEACTIVATE:
      Message deactivate1 = {Type::TAG1, State::DEACTIVATE};
      Message deactivate2 = {Type::TAG2, State::DEACTIVATE};
      esp_now_send(broadcastAddress, (uint8_t*)&deactivate1, sizeof(Message));
      esp_now_send(broadcastAddress, (uint8_t*)&deactivate2, sizeof(Message));
      #ifdef DEBUG
      Serial.println("ACTIVE");
      #endif
      break;
    case State::CHECK_LIGHT:
      // TODO: Code here
      break;
    
    default:
      break;
    }
  }
}

void setup() {
  #ifdef DEBUG
  Serial.begin(115200);
  #endif

  // Turn on virtual ground pins BEFORE starting the LCD
  pinMode(25, OUTPUT);
  digitalWrite(25, LOW);  // A1 -> Contrast (V0)
  pinMode(26, OUTPUT);
  digitalWrite(26, LOW);  // A0 -> Write-only (RW)
  pinMode(21, OUTPUT);
  digitalWrite(21, LOW);  // 21 -> Backlight - (K)

  lcd.begin(16, 2);

  // Wifi setup
  setupWifi(broadcastAddress, onReceive);

  // Begin start animation
  set_rendering_state(START_ANIMATION);
}

void loop() {
  // other logic goes here
  //
  //
  Message msg = {Type::CENTRAL_UNIT, State::ACTIVATE};

  esp_now_send(broadcastAddress, (uint8_t*)&msg, sizeof(msg));

  // Begin rendering phase
  uint32_t state_elapsed_ms = millis() - state_start_ms;

  switch (current_rendering_state) {
    case START_ANIMATION: {

      start_animation(state_elapsed_ms);

      break;
    }
    case PASSIVE:
      // TODO: passive visuals
      break;
    case TAG_EDITING:
      // TODO: tag editing visuals
      break;
  }
}