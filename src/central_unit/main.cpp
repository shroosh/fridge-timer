#define DEBUG 0

#include <Arduino.h>
#include <LiquidCrystal.h>

#include "../include/connection.hpp"
#include "symbol.hpp"
#include "B_anim_path.cpp"

// Enum for different UI states, which tells loop function what code to execute
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

// Tracks how many animation stages have been completed
size_t start_anim_stages_completed = 0;


OpacitySymbol B_parts[4] = {};
uint8_t B_buffers[4][8] = {0};
uint8_t B_CGRAM_slots[4] = {0, 1, 2, 3};


// Pins match: RS=13, E=27, D4=33, D5=15, D6=32, D7=14
LiquidCrystal lcd(13, 27, 33, 15, 32, 14);

const uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
constexpr uint32_t SECONDS_PER_DAY = 86400UL;
constexpr uint8_t MAX_TIMER_DAYS = 99;
constexpr uint8_t BUTTON_SELECT_PIN = 18;
constexpr uint8_t BUTTON_PLUS_PIN = 19;
constexpr uint8_t BUTTON_MINUS_PIN = 22;
constexpr uint8_t BUTTON_RESET_PIN = 23;

struct Timer {
  Type tag;
  uint8_t setDays;
  uint32_t remainingSeconds;
  uint32_t lastTickMs;
  bool running;
  bool hasSentState;
  bool lastSentWarning;
  bool hasAttemptedSend;
  uint32_t lastSendAttemptMs;
};

struct Button {
  uint8_t pin;
  bool lastReading;
  bool stableReading;
  uint32_t lastChangeMs;
};

Timer timers[2] = {
  {Type::TAG1, 0, 0, 0, false, false, false, false, 0},
  {Type::TAG2, 0, 0, 0, false, false, false, false, 0}
};
Button buttons[4] = {
  {BUTTON_SELECT_PIN, HIGH, HIGH, 0},
  {BUTTON_PLUS_PIN, HIGH, HIGH, 0},
  {BUTTON_MINUS_PIN, HIGH, HIGH, 0},
  {BUTTON_RESET_PIN, HIGH, HIGH, 0}
};
uint8_t selected_timer = 0;
bool timer_screen_dirty = true;

void set_rendering_state(RenderingState new_state) {
  current_rendering_state = new_state;
  state_start_ms = millis();

  lcd.clear();
}

void beginButton(Button& button) {
  pinMode(button.pin, INPUT_PULLUP);
  button.lastReading = digitalRead(button.pin);
  button.stableReading = button.lastReading;
  button.lastChangeMs = millis();
}

bool wasPressed(Button& button, uint32_t now) {
  bool reading = digitalRead(button.pin);
  if (reading != button.lastReading) {
    button.lastReading = reading;
    button.lastChangeMs = now;
  }

  if (now - button.lastChangeMs >= 30 &&
      button.stableReading != button.lastReading) {
    button.stableReading = button.lastReading;
    return button.stableReading == LOW;
  }
  return false;
}

void restartTimer(Timer& timer, uint32_t now) {
  timer.remainingSeconds = static_cast<uint32_t>(timer.setDays) * SECONDS_PER_DAY;
  timer.lastTickMs = now;
  timer.running = true;
  timer_screen_dirty = true;
}

void resetSelectedTimer(uint32_t now) {
  Timer& timer = timers[selected_timer];
  timer.setDays = 0;
  timer.remainingSeconds = 0;
  timer.running = false;
  timer.lastTickMs = now;
  timer_screen_dirty = true;
}

void handleButtons(uint32_t now) {
  if (wasPressed(buttons[0], now)) {
    selected_timer = (selected_timer + 1) % 2;
    timer_screen_dirty = true;
  }

  Timer& timer = timers[selected_timer];
  if (wasPressed(buttons[1], now) && timer.setDays < MAX_TIMER_DAYS) {
    ++timer.setDays;
    restartTimer(timer, now);
  }
  if (wasPressed(buttons[2], now) && timer.setDays > 1) {
    --timer.setDays;
    restartTimer(timer, now);
  }
  if (wasPressed(buttons[3], now)) {
    resetSelectedTimer(now);
  }
}

void updateTimers(uint32_t now) {
  for (Timer& timer : timers) {
    if (!timer.running) continue;

    while (now - timer.lastTickMs >= 1000UL) {
      timer.lastTickMs += 1000UL;
      if (timer.remainingSeconds > 0) {
        --timer.remainingSeconds;
        timer_screen_dirty = true;
      }
    }
  }
}

void sendTimerStates(uint32_t now) {
  for (Timer& timer : timers) {
    bool warning = timer.running && timer.remainingSeconds < SECONDS_PER_DAY;
    bool stateChanged = !timer.hasSentState || warning != timer.lastSentWarning;
    bool retryDue = timer.hasAttemptedSend &&
                    now - timer.lastSendAttemptMs >= 5000UL;
    if (!stateChanged && !retryDue) continue;
    if (timer.hasAttemptedSend && now - timer.lastSendAttemptMs < 1000UL) continue;

    Message msg = {
      timer.tag,
      warning ? State::ACTIVATE : State::DEACTIVATE
    };
    timer.lastSendAttemptMs = now;
    timer.hasAttemptedSend = true;
    esp_err_t result = esp_now_send(
      broadcastAddress, reinterpret_cast<const uint8_t*>(&msg), sizeof(msg));
    if (result == ESP_OK) {
      timer.hasSentState = true;
      timer.lastSentWarning = warning;
    } else {
      Serial.printf("ESP-NOW send failed for tag %u: %d\n",
                    timer.tag == Type::TAG1 ? 1 : 2, result);
    }
  }
}

void drawTimerScreen() {
  for (uint8_t i = 0; i < 2; ++i) {
    const Timer& timer = timers[i];
    char line[17] = "                ";
    char marker = i == selected_timer ? '>' : ' ';
    line[0] = marker;
    line[1] = static_cast<char>('1' + i);
    line[2] = ' ';

    if (!timer.running) {
      line[3] = 'O';
      line[4] = 'F';
      line[5] = 'F';
    } else {
      uint32_t days = timer.remainingSeconds / SECONDS_PER_DAY;
      uint32_t hours = (timer.remainingSeconds % SECONDS_PER_DAY) / 3600UL;
      uint32_t minutes = (timer.remainingSeconds % 3600UL) / 60UL;
      uint32_t seconds = timer.remainingSeconds % 60UL;
      if (days > MAX_TIMER_DAYS) days = MAX_TIMER_DAYS;
      line[3] = static_cast<char>('0' + days / 10);
      line[4] = static_cast<char>('0' + days % 10);
      line[5] = 'D';
      line[6] = ' ';
      line[7] = static_cast<char>('0' + hours / 10);
      line[8] = static_cast<char>('0' + hours % 10);
      line[9] = ':';
      line[10] = static_cast<char>('0' + minutes / 10);
      line[11] = static_cast<char>('0' + minutes % 10);
      line[12] = ':';
      line[13] = static_cast<char>('0' + seconds / 10);
      line[14] = static_cast<char>('0' + seconds % 10);
    }

    lcd.setCursor(0, i);
    lcd.print(line);
  }
  timer_screen_dirty = false;
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

    start_anim_stages_completed = 1;
  } else if (elapsed_ms < 4250) { // "EST" appears

    if (start_anim_stages_completed == 1) {
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

      start_anim_stages_completed = 2;
    }

  } else if (elapsed_ms < 5000) { // "EFORE" appears

    if (start_anim_stages_completed == 2) {
      lcd.clear();
      // Write characters to screen
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

      start_anim_stages_completed = 3;
    }

  } else { // "RO" appears
    if (start_anim_stages_completed == 3) {
      lcd.setCursor(14,1);
      lcd.print("RO");
      start_anim_stages_completed = 4;
    }
  }
}



void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(Message)) return;

  Message msg;
  memcpy(&msg, data, sizeof(msg));
  if (msg.destination == Type::CENTRAL_UNIT) {
    #ifdef DEBUG
    Serial.printf("Tag reported state %u\n", static_cast<unsigned>(msg.state));
    #endif
  }
}

void setup() {
  Serial.begin(115200);

  // Turn on virtual ground pins BEFORE starting the LCD
  pinMode(25, OUTPUT);
  digitalWrite(25, LOW);  // A1 -> Contrast (V0)
  pinMode(26, OUTPUT);
  digitalWrite(26, LOW);  // A0 -> Write-only (RW)
  pinMode(21, OUTPUT);
  digitalWrite(21, LOW);  // 21 -> Backlight - (K)

  lcd.begin(16, 2);

  for (Button& button : buttons) beginButton(button);
  uint32_t now = millis();
  for (Timer& timer : timers) timer.lastTickMs = now;

  // Wifi setup
  esp_err_t wifiResult = setupWifi(broadcastAddress, onReceive);
  if (wifiResult != ESP_OK) {
    Serial.printf("ESP-NOW setup failed: %d\n", wifiResult);
  }

  // Begin start animation
  set_rendering_state(START_ANIMATION);
}

void loop() {
  uint32_t now = millis();
  handleButtons(now);
  updateTimers(now);
  sendTimerStates(now);

  // Begin rendering phase
  uint32_t state_elapsed_ms = now - state_start_ms;

  switch (current_rendering_state) {
    case START_ANIMATION: {
      start_animation(state_elapsed_ms);
      if (state_elapsed_ms >= 5000UL) {
        set_rendering_state(PASSIVE);
        timer_screen_dirty = true;
      }
      break;
    }
    case PASSIVE:
      if (timer_screen_dirty) drawTimerScreen();
      break;
    case TAG_EDITING:
      if (timer_screen_dirty) drawTimerScreen();
      break;
  }
}