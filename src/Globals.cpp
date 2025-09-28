// Globals.cpp
#include "Globals.h"

volatile uint8_t brightness = 128;  // 0..255, LED brightness
volatile uint8_t ledUpTo = 0;     // 0..NUMPIXELS (60), number of active LEDs
volatile UIMode uiMode = UIMode::STATIC;  // LED strip animation mode
volatile bool ledEnabled = false;    // LED strip on/off state

volatile uint16_t hue = 0;          // 0..255, color hue
volatile uint8_t sat = 255;        // 0..255, color saturation
volatile float freq = 0.5f;        // Hz, animation frequency
volatile float duty = 50.0f;       // 0..100%, duty cycle


