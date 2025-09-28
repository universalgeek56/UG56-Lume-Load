// NetManager.h
#pragma once
#include <Arduino.h>

// Network and OTA management
namespace NetManager {
    void begin(); // Initialize Wi-Fi and OTA
    void update(); // Handle OTA updates
}
