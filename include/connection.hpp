#include <esp_now.h>
#include <WiFi.h>

enum class State {ACTIVATE, DEACTIVATE, CHECK_LIGHT};
enum class Type {CENTRAL_UNIT, TAG};

typedef struct {
  Type type;   // 0 = from central, 1 = from node
  State state;
} Message;

void setupWifi(const uint8_t (&mac)[6], esp_now_recv_cb_t onRecieve) {
    WiFi.mode(WIFI_STA);

    esp_now_init();

    esp_now_register_recv_cb(onRecieve);

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, mac, 6);
    peer.channel = 0;
    peer.encrypt = false;
    esp_now_add_peer(&peer);
}