// main.cpp
#include <Arduino.h>
#include "Config.h"
#include "Globals.h"
#include "NetManager.h"
#include "DemoManager.h"
#include "WebInterface.h"

void setup() {
    NetManager::begin();
    WebInterface::begin();
    DemoManager::begin();
}

void loop() {
    NetManager::update();
    WebInterface::update();
    DemoManager::update();
}
