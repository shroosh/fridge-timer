#include <esp_now.h>
#include <WiFi.h>

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct {
  uint8_t type;   // 0 = from central, 1 = from node
  bool state;
} Message;

const int LED_PIN = BUILTIN_LED;     // HUZZAH32 onboard LED
const int BUTTON_PIN = 21;   // A5/4

// Core 3.x signature. For core 2.x use: (const uint8_t *mac, const uint8_t *data, int len)
void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  Message msg;
  memcpy(&msg, data, sizeof(msg));
  if (msg.type == 1) {
    digitalWrite(LED_PIN, msg.state);
    Serial.printf("[RX] node button %s -> LED %s\n",
      msg.state ? "pressed" : "released", msg.state ? "ON" : "OFF");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  Serial.println("\n=== CENTRAL ===");
  Serial.print("My MAC: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init FAILED");
    return;
  }
  esp_now_register_recv_cb(onReceive);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, broadcastAddress, 6);
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

    Message msg = {0, button == LOW};
    Serial.printf("[BTN] %s -> telling nodes %s\n",
      button == LOW ? "pressed" : "released", msg.state ? "ON" : "OFF");
    esp_now_send(broadcastAddress, (uint8_t *)&msg, sizeof(msg));
  }
}