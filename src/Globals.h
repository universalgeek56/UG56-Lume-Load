// Globals.h
#pragma once
#include <Arduino.h>

enum class UIMode : uint8_t {
    STATIC = 1,
    SINE = 2,
    RECT = 3,
    SAW = 4,
    TRIANGLE = 5
};

extern volatile uint8_t brightness;   // 0..255, LED brightness
extern volatile uint8_t ledUpTo;      // 0..NUMPIXELS (60), number of active LEDs
extern volatile UIMode uiMode;        // LED strip animation mode
extern volatile bool ledEnabled;       // LED strip on/off state

extern volatile uint16_t hue;         // 0..255, color hue
extern volatile uint8_t sat;          // 0..255, color saturation
extern volatile float freq;           // Hz, animation frequency
extern volatile float duty;           // 0..100%, duty cycle