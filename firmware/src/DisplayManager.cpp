/*

Copyright 2025 Marc Ketel
SPDX-License-Identifier: Apache-2.0

*/

#include "DisplayManager.h"
#include "Colors.h"

// Must not be too bright. There is only 0.5A available.
uint8_t DisplayManager::brightness = 75;

/**
 * Constructs a DisplayManager instance for the LED strip.
 * Initializes the NeoPixel strip.
 */
DisplayManager::DisplayManager(uint8_t dataPin)
    : strip(PixelCount, dataPin) {
}

/**
 * Initializes the LED strip display.
 * Begins the NeoPixel strip and shows all blue LEDs.
 */
void DisplayManager::initialize() {
    strip.Begin();
    strip.SetLuminance(brightness);
    all(COLOR_BLUE);
    strip.Show();
}

/**
 * Displays a number on the 7-segment LED display.
 * Shows the number with green color if open, red otherwise; handles out-of-range values.
 */
void DisplayManager::displayNumber(int16_t n, bool open) {
    if (n < 0) {
        showText(" -- ", COLOR_YELLOW);
        return;
    }

    if (n > 9999) {
        showText(" HH ", COLOR_YELLOW);
        return;
    }

    RgbColor colorOpenClose = open ? COLOR_GREEN : COLOR_RED;
    RgbColor color;

    // pos 0: thousands (MSD)
    uint8_t pos0_pat;
    if (n > 999) {
        pos0_pat = getPattern('0' + (n / 1000));
        color = colorOpenClose;
    } else {
        pos0_pat = getPattern('n'); // Assuming 'n' for blank or adjust
        color = COLOR_YELLOW;
    }
    showPattern(0, pos0_pat, color);

    // pos 1: hundreds
    uint8_t pos1_pat;
    if (n > 99) {
        pos1_pat = getPattern('0' + ((n % 1000) / 100));
        color = colorOpenClose;
    } else {
        pos1_pat = getPattern('='); // blank
        color = COLOR_YELLOW;
    }
    showPattern(1, pos1_pat, color);

    // pos 2: tens
    uint8_t pos2_pat;
    color = colorOpenClose;
    if (n > 9) {
        pos2_pat = getPattern('0' + ((n % 100) / 10));
    } else {
        pos2_pat = 0; // blank
    }
    showPattern(2, pos2_pat, color);

    // pos 3: units (LSD)
    uint8_t pos3_pat = getPattern('0' + (n % 10));
    color = colorOpenClose;
    // Set dp for last digit if !open
    if (!open) {
        pos3_pat |= 1 << 7; // bit 7 for dp
    }
    showPattern(3, pos3_pat, color);

    strip.Show();
}

/**
 * Displays a 7-segment pattern for a specific digit position using the specified color.
 * Lights up segments based on the pattern bits using predefined pixel mappings.
 */
void DisplayManager::showPattern(uint8_t digit, uint8_t pattern, RgbColor color) {
    uint16_t base = (3 - digit) * 15; // LSD digit3 at 0-14, MSD digit0 at 45-59

    // Clear all 15 LEDs of this digit to off (black)
    for (int i = 0; i < 15; ++i) {
        strip.SetPixelColor(base + i, RgbColor(0, 0, 0));
    }

    // For each of the 8 bits in the pattern (bit0=dp, bit1=a, ..., bit7=g):
    for (uint8_t bit = 0; bit < 8; ++bit) {
        // If the bit is set (segment should be on), light the corresponding 1-2 pixels using the lookup table
        if (pattern & (1 << bit)) { // Check if this segment bit is set in the pattern
            // For each pixel in this segment (most have 2, dp has 1 with second as sentinel 0xFFFF)
            for (uint8_t p = 0; p < 2; ++p) {
                uint16_t offset = segmentPixels[7 - bit][p];   // Get the relative pixel offset from base
                if (offset != 0xFFFF) {                        // Skip sentinel value for single-LED segments like dp
                    strip.SetPixelColor(base + offset, color); // Light this pixel
                }
            }
        }
    }
}

/**
 * Retrieves the 7-segment pattern for a given character.
 * Maps ASCII character to predefined pattern index or returns undefined if invalid.
 */
uint8_t DisplayManager::getPattern(char character) {
    int index = (int)character - 32;
    if (index >= 0 && index < 96) {
        return patterns[index];
    }
    return PATTERN_UNDEFINED;
}

/**
 * Displays text on the 4-digit LED display using the specified color.
 * Shows each character using patterns and blanks unused digits.
 */
void DisplayManager::showText(const char *text, RgbColor color) {
    for (uint8_t i = 0; i < 4 && text[i] != '\0'; ++i) {
        uint8_t pat = getPattern(text[i]);
        showPattern(i, pat, color);
    }
    // Blank remaining digits
    for (uint8_t i = 0; i < 4; ++i) {
        bool found = false;
        for (uint8_t j = 0; j < 4 && text[j] != '\0'; ++j) {
            if (i == j) {
                found = true;
                break;
            }
        }
        if (!found) {
            showPattern(i, 0, color);
        }
    }
    strip.Show();
}

/**
 * Sets all LEDs to the specified color.
 * Applies dimming to the entire strip.
 */
void DisplayManager::all(RgbColor color) {
    for (uint16_t i = 0; i < PixelCount; ++i) {
        strip.SetPixelColor(i, color);
    }
}

/**
 * Static array defining pixel offsets for each 7-segment bit.
 */
const uint16_t DisplayManager::segmentPixels[8][2] = {
    {2, 0xFFFF}, // bit0=dp: pixel 2 (single)
    {11, 12},    // bit1=a: pixels 11-12
    {9, 10},     // bit2=b: 9-10
    {0, 1},      // bit3=c: 0-1
    {3, 4},      // bit4=d: 3-4
    {5, 6},      // bit5=e: 5-6
    {13, 14},    // bit6=f: 13-14
    {7, 8}       // bit7=g: 7-8
};

/**
 * 7-segment led to bit mapping: dp, a, b, c, d, e, f, g.
 */
const uint8_t DisplayManager::patterns[96] = {
    0b00000000,        // space
    PATTERN_UNDEFINED, // !
    PATTERN_UNDEFINED, // "
    PATTERN_UNDEFINED, // #
    PATTERN_UNDEFINED, // $
    PATTERN_UNDEFINED, // %
    PATTERN_UNDEFINED, // &
    PATTERN_UNDEFINED, // '
    PATTERN_UNDEFINED, // (
    PATTERN_UNDEFINED, // )
    PATTERN_UNDEFINED, // *
    PATTERN_UNDEFINED, // +
    PATTERN_UNDEFINED, // ,
    0b00000001,        // -
    PATTERN_UNDEFINED, // .
    PATTERN_UNDEFINED, // /
    0b01111110,        // 0
    0b00110000,        // 1
    0b01101101,        // 2
    0b01111001,        // 3
    0b00110011,        // 4
    0b01011011,        // 5
    0b01011111,        // 6
    0b01110000,        // 7
    0b01111111,        // 8
    0b01111011,        // 9
    PATTERN_UNDEFINED, // :
    PATTERN_UNDEFINED, // ;
    PATTERN_UNDEFINED, // <
    0b00001001,        // =
    PATTERN_UNDEFINED, // >
    PATTERN_UNDEFINED, // ?
    PATTERN_UNDEFINED, // @
    0b01110111,        // A
    PATTERN_UNDEFINED, // B
    PATTERN_UNDEFINED, // C
    PATTERN_UNDEFINED, // D
    PATTERN_UNDEFINED, // E
    0b01000111,        // F
    PATTERN_UNDEFINED, // G
    0b00110111,        // H
    0b00010000,        // I
    PATTERN_UNDEFINED, // J
    PATTERN_UNDEFINED, // K
    0b00001110,        // L
    0b01110110,        // N
    PATTERN_UNDEFINED, // M
    0b01111110,        // O
    0b01100111,        // P
    0b01110011,        // Q
    PATTERN_UNDEFINED, // R
    0b00001111,        // T
    PATTERN_UNDEFINED, // S
    PATTERN_UNDEFINED, // U
    0b00011100,        // V
    0b00111111,        // W
    PATTERN_UNDEFINED, // X
    PATTERN_UNDEFINED, // Y
    PATTERN_UNDEFINED, // Z
    0b01001110,        // [
    PATTERN_UNDEFINED, // backslash
    0b01111000,        // ]
    PATTERN_UNDEFINED, // ^
    0b00001000,        // _
    0b00000010,        // `
    0b01110111,        // a
    PATTERN_UNDEFINED, // b
    PATTERN_UNDEFINED, // c
    PATTERN_UNDEFINED, // d
    PATTERN_UNDEFINED, // e
    PATTERN_UNDEFINED, // f
    PATTERN_UNDEFINED, // g
    PATTERN_UNDEFINED, // h
    PATTERN_UNDEFINED, // i
    PATTERN_UNDEFINED, // j
    PATTERN_UNDEFINED, // k
    0b00001100,        // l
    PATTERN_UNDEFINED, // m
    0b00010101,        // n
    0b00011101,        // o
    PATTERN_UNDEFINED, // p
    PATTERN_UNDEFINED, // q
    PATTERN_UNDEFINED, // r
    PATTERN_UNDEFINED, // s
    0b00001111,        // t
    PATTERN_UNDEFINED, // u
    PATTERN_UNDEFINED, // v
    0b00111111,        // w
    PATTERN_UNDEFINED, // x
    PATTERN_UNDEFINED, // y
    PATTERN_UNDEFINED, // z
    PATTERN_UNDEFINED, // {
    0b00000110,        // |
    PATTERN_UNDEFINED, // }
    0b01000000,        // ~
    0b00000000,        // (del)
};

