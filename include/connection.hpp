#include <esp_now.h>
#include <WiFi.h>
#include <string.h>

enum class State : uint8_t {ACTIVATE, DEACTIVATE, CHECK_LIGHT};
enum class Type : uint8_t {CENTRAL_UNIT, TAG1, TAG2};

struct Message {
  Type destination;
  State state;
};

esp_err_t setupWifi(const uint8_t (&mac)[6], esp_now_recv_cb_t onReceive) {
  if (!WiFi.mode(WIFI_STA)) return ESP_FAIL;

  esp_err_t result = esp_now_init();
  if (result != ESP_OK) return result;

  result = esp_now_register_recv_cb(onReceive);
  if (result != ESP_OK) return result;

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  return esp_now_add_peer(&peer);
}