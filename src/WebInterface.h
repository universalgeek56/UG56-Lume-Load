// WebInterface.h
#pragma once
#include <Arduino.h>

namespace WebInterface {
    void begin();
    void update();
    void sendLog(const String& msg);
}

