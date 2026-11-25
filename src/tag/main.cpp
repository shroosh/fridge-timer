#include <esp_now.h>
#include <WiFi.h>

uint8_t centralAddress[] = {0x44, 0x17, 0x93, 0x5C, 0x0A, 0x70}; // CHANGE ME

typedef struct {
  uint8_t type;   // 0 = from central, 1 = from node
  bool state;
} Message;

const int LED_PIN = BUILTIN_LED;
const int BUTTON_PIN = 21;   // A5/4

// Core 3.x signature. For core 2.x use: (const uint8_t *mac, const uint8_t *data, int len)
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  Message msg;
  memcpy(&msg, data, sizeof(msg));
  if (msg.type == 0) {
    digitalWrite(LED_PIN, msg.state);
    Serial.printf("[RX] central button %s -> LED %s\n",
      msg.state ? "pressed" : "released", msg.state ? "ON" : "OFF");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  Serial.println("\n=== NODE ===");
  Serial.print("My MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init FAILED");
    return;
  }
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, centralAddress, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_now_add_peer(&peer);
  Serial.println("Ready.");
}

void loop() {
  static bool lastButton = HIGH;
  static unsigned long lastChange = 0;

  bool button = digitalRead(BUTTON_PIN);
  if (button != lastButton && millis() - lastChange > 30) {
    lastChange = millis();
    lastButton = button;

    Message msg = {1, button == LOW};
    Serial.printf("[BTN] %s -> telling central %s\n",
      button == LOW ? "pressed" : "released", msg.state ? "ON" : "OFF");
    esp_now_send(centralAddress, (uint8_t *)&msg, sizeof(msg));
  }
}

//uint8_t centralAddress[] = {0x44, 0x17, 0x93, 0x5C, 0x0A, 0x70}; // CHANGE ME


// #include <Arduino.h>

// #define DEBUG 0

// const int LDR_PIN = 34;
// const int LED_PIN = 21;
// const int lightTreshold = 2000;
// const int SPEAKER_PIN = 99999; // TODO: hitta på pin.

// void setup() {
//   Serial.begin(115200);

//   // LDR
//   analogSetAttenuation(ADC_11db);

//   // LED
//   pinMode(LED_PIN, OUTPUT);

//   // SPEAKER
//   ledcAttach(SPEAKER_PIN, 2000, 8);
//   ledcWriteTone(SPEAKER_PIN, 0);
// }

// // Central unit med grön sticker:   44:17:93:5C:0A:70

// void loop() {
  
// }

// bool isBright() {
//   int lightLevel = analogReadMilliVolts(LDR_PIN);

//   if (lightLevel <  lightTreshold) {
//     return true;
//   } else {
//     return false;
//   }

//   #ifdef DEBUG
//   Serial.printf("%d mV\n", lightLevel);
//   #endif
// }

// void stopAlarm() {
//   digitalWrite(LED_PIN, LOW);
//   ledcWriteTone(SPEAKER_PIN, 0);
// }

// void alarm() {
//   digitalWrite(LED_PIN, HIGH);
//   ledcWriteTone(SPEAKER_PIN, 3500);

//   delay(500);
//   ledcWriteTone(SPEAKER_PIN, 0);
//   delay(500);
// }