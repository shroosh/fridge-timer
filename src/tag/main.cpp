#include "../include/connection.hpp"
#define DEBUG 0
// Constants
const uint8_t centralAddress[] = {0x44, 0x17, 0x93, 0x5C, 0x0A, 0x70};  // MAC

const int LED_PIN = 21;                            // Pin number
const int SPEAKER_PIN = 23;                        // Pin number
const int LDR_PIN = 34;                            // Pin number
const int LIGHT_TRESHOLD = 1500;                   // mV
const unsigned long BEEP_INTERVAL = 500;           // ms
const int BEEP_FREQ = 3500;                        // Hz
const unsigned long LIGHT_CHANGE_INTERVAL = 2000;  // ms
#ifndef TAG_ID
#define TAG_ID 1
#endif
#if TAG_ID == 1
const Type MY_TAG = Type::TAG1;
#elif TAG_ID == 2
const Type MY_TAG = Type::TAG2;
#else
#error "TAG_ID must be 1 or 2"
#endif
const Message ACTIVATE_MSG = {Type::CENTRAL_UNIT, State::ACTIVATE};
const Message DEACTIVATE_MSG = {Type::CENTRAL_UNIT, State::DEACTIVATE};

// const uint_8 BRIGHT_MESSAGE

// VARIABLES
unsigned long lastToggle = 0;
unsigned long lastLightChange = 0;
bool speakerOn = false;
State current_state = State::DEACTIVATE;
bool bright = false;

void onReceive(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (len != sizeof(Message)) return;

  Message msg;
  memcpy(&msg, data, sizeof(msg));
  if (msg.destination == MY_TAG) {
    switch (msg.state) {
      case State::ACTIVATE:
        current_state = State::ACTIVATE;
        break;
      case State::DEACTIVATE:
        current_state = State::DEACTIVATE;
        break;
      case State::CHECK_LIGHT:
        /* code */
        break;

      default:
        break;
    }
  }
}

bool isBright() {
  int lightLevel = analogReadMilliVolts(LDR_PIN);

#ifdef DEBUG
  Serial.printf("%d mV\n", lightLevel);
#endif

  return lightLevel < LIGHT_TRESHOLD;
}

void updateSpeaker() {
  if (current_state != State::ACTIVATE || !bright) {
    digitalWrite(LED_PIN, LOW);
    if (speakerOn) {
      ledcWriteTone(SPEAKER_PIN, 0);
      speakerOn = false;
    }
    return;
  }

  unsigned long now = millis();
  if (now - lastToggle >= BEEP_INTERVAL) {
    lastToggle = now;
    speakerOn = !speakerOn;
    digitalWrite(LED_PIN, speakerOn ? HIGH : LOW);
    ledcWriteTone(SPEAKER_PIN, speakerOn ? BEEP_FREQ : 0);
  }
}

void setup() {
  Serial.begin(115200);

  // WIFI
  esp_err_t wifiResult = setupWifi(centralAddress, onReceive);
  if (wifiResult != ESP_OK) {
    Serial.printf("ESP-NOW setup failed: %d\n", wifiResult);
  }

#ifdef DEBUG
  delay(300);
  Serial.println(WiFi.macAddress());
#endif

  // LDR
  analogSetAttenuation(ADC_11db);

  // LED
  pinMode(LED_PIN, OUTPUT);

  // SPEAKER
  ledcAttach(SPEAKER_PIN, 2000, 8);
  ledcWriteTone(SPEAKER_PIN, 0);
}

void loop() {
  updateSpeaker();

  unsigned long now = millis();
  if (now - lastLightChange >= LIGHT_CHANGE_INTERVAL) {
    lastLightChange = now;
    bool brightNow = isBright();
    if (brightNow != bright) {
      bright = brightNow;
#ifdef DEBUG
      Serial.println("Light change detected!!! Sending message to CU");
#endif
      const Message& msg = bright ? ACTIVATE_MSG : DEACTIVATE_MSG;
      esp_now_send(centralAddress, (uint8_t*)&msg, sizeof(msg));
    }
  }
}