// DemoManager.cpp
#include "DemoManager.h"
#include "Globals.h"
#include "Config.h"
#include <Adafruit_NeoPixel.h>
#include <math.h>

namespace DemoManager {
namespace {
    Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
    unsigned long lastUpdate = 0;

    // Constants
    const uint32_t UPDATE_INTERVAL_MS = 20; // ~50Hz update rate

    // HSV to RGB conversion (hue: 0-255, sat/val: 0-255)
    void hsvToRgb(uint16_t hue, uint8_t sat, uint8_t val, uint8_t& r, uint8_t& g, uint8_t& b) {
        float h = hue / 42.5f; // 255 / 6 = 42.5
        int i = static_cast<int>(h);
        float f = h - i;
        float p = val * (1.0f - sat / 255.0f);
        float q = val * (1.0f - sat / 255.0f * f);
        float t = val * (1.0f - sat / 255.0f * (1.0f - f));

        switch (i % 6) {
            case 0: r = val; g = t; b = p; break;
            case 1: r = q; g = val; b = p; break;
            case 2: r = p; g = val; b = t; break;
            case 3: r = p; g = q; b = val; break;
            case 4: r = t; g = p; b = val; break;
            case 5: r = val; g = p; b = q; break;
            default: r = g = b = 0; break;
        }
    }

    // Calculate brightness value based on mode
    uint8_t calculateBrightness(float t, UIMode mode) {
        switch (mode) {
            case UIMode::STATIC:
                return brightness;
            case UIMode::SINE:
                return static_cast<uint8_t>(brightness * (0.5f + 0.5f * sin(TWO_PI * freq * t)));
            case UIMode::RECT:
                return (fmod(t * freq, 1.0f) < (duty / 100.0f)) ? brightness : 0;
            case UIMode::SAW:
                return static_cast<uint8_t>(fmod(t * freq, 1.0f) * brightness);
            case UIMode::TRIANGLE:
                return static_cast<uint8_t>((1.0f - fabs(fmod(t * freq, 2.0f) - 1.0f)) * brightness);
            default:
                return 0;
        }
    }
} // anonymous namespace

void begin() {
    pixels.begin();
    pixels.show();
}

void update() {
    unsigned long now = millis();
    if (now - lastUpdate < UPDATE_INTERVAL_MS) return;
    lastUpdate = now;

    if (!ledEnabled) {
        pixels.clear();
        pixels.show();
        return;
    }

    int upTo = constrain(ledUpTo, 0, NUMPIXELS);
    float t = now / 1000.0f;

    for (int i = 0; i < NUMPIXELS; ++i) {
        uint8_t val = (i < upTo) ? calculateBrightness(t, uiMode) : 0;
        uint8_t r, g, b;
        hsvToRgb(hue, sat, val, r, g, b);
        pixels.setPixelColor(i, pixels.Color(r, g, b));
    }

    pixels.show();
}
} // namespace DemoManager