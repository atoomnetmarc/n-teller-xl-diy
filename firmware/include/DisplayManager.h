/*

Copyright 2025 Marc Ketel
SPDX-License-Identifier: Apache-2.0

*/

#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include "Colors.h"
#include <NeoPixelBusLg.h>

class DisplayManager {
public:
    DisplayManager(uint8_t dataPin);
    void initialize();
    void displayNumber(int16_t n, bool open = true);
    void showPattern(uint8_t digit, uint8_t pattern, RgbColor color);
    uint8_t getPattern(char character);
    void showText(const char *text, RgbColor color);
    static uint8_t brightness;

private:
    NeoPixelBusLg<NeoGrbFeature, NeoEsp8266Uart1Sk6812Method, NeoGammaNullMethod> strip;
    static constexpr uint16_t PixelCount = 60;
    static const uint8_t PATTERN_UNDEFINED = 0b11001001;
    static const uint8_t patterns[96];
    static const uint16_t segmentPixels[8][2];
    void all(RgbColor color);
};

#endif