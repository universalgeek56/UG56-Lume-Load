// NetManager.cpp
#include "NetManager.h"
#include "Config.h"
#include <ESP8266WiFi.h>
#include <ArduinoOTA.h>

namespace NetManager {
namespace {
    const uint32_t WIFI_TIMEOUT_MS = 15000; // 15 sec for initial connect
    const uint32_t RECONNECT_INTERVAL_MS = 10000; // try every 10 sec if dropped
    uint32_t lastReconnectAttempt = 0;
}

void begin() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
        delay(500);
    }

    ArduinoOTA.setHostname(OTA_HOSTNAME);
    ArduinoOTA.begin();
}

void update() {
    ArduinoOTA.handle();

    if (WiFi.status() != WL_CONNECTED) {
        uint32_t now = millis();
        if (now - lastReconnectAttempt >= RECONNECT_INTERVAL_MS) {
            lastReconnectAttempt = now;
            WiFi.disconnect();
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        }
    }
}
} // namespace NetManager
