#pragma once

#include <Arduino.h>
#include <FastLED.h>

struct PinConfig {
    uint8_t pin;
    uint8_t num_strips;
    uint16_t leds_per_strip;
    uint16_t total_leds;
    CRGB* led_array;
};

enum PatternType {
    PATTERN_RAINBOW_CHASE,
    PATTERN_WHITE_CHASE,
    PATTERN_SOLID_COLOR,
    PATTERN_OFF,
    PATTERN_FLASHBULB,
    PATTERN_PINWHEEL,
    PATTERN_CUSTOM
};

struct ColorPalette {
    CRGB colors[8];
    uint8_t size;
};

struct StripState {
    PatternType pattern;
    uint32_t start_time;
    uint32_t duration; // 0 = infinite
    CRGB color;
    bool active;
    bool reverse; // if true, patterns run from last index to 0
    ColorPalette palette; // for patterns that use color palettes
    uint8_t pinwheel_group_id; // for pinwheel patterns, which group this strip belongs to
};

class StripPatternManager {
public:
    static void init(uint8_t total_strips);
    static void setStripPattern(uint8_t strip_id, PatternType pattern, CRGB color = CRGB::White, uint32_t duration = 0);
    static void setStripPatternWithDelay(
        uint8_t strip_id, PatternType pattern, CRGB color, uint32_t delay_ms, uint32_t duration = 0);
    static void setPinwheelPattern(uint8_t* strip_ids, uint8_t num_strips, ColorPalette palette, uint32_t duration = 0);
    static void setFlashBulbPattern(uint8_t* strip_ids, uint8_t num_strips);
    static void updateAllStrips(
        PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t num_pins, uint32_t current_time);
    static void clearStrip(uint8_t strip_id);
    static void clearAllStrips();
    static StripState* getStripState(uint8_t strip_id);
    static void setStripReverse(uint8_t strip_id, bool reverse);

private:
    static StripState* strip_states;
    static uint8_t total_strip_count;
    static uint32_t global_rainbow_time;

    static void renderStrip(uint8_t strip_id, uint8_t pin, uint8_t strip_on_pin, uint16_t strip_length, CRGB* led_array,
        uint16_t led_offset, uint32_t current_time);
    static uint8_t getStripId(uint8_t pin, uint8_t strip_on_pin);
    static void getStripPosition(uint8_t strip_id, uint8_t& pin, uint8_t& strip_on_pin);
    static uint16_t transformLedIndex(uint8_t strip_id, uint16_t led_index, uint16_t strip_length);
    static void getStripCoordinates(uint8_t strip_id, float& center_x, float& center_y);
    static void getLedMatrixPosition(uint8_t strip_id, uint16_t led_index, float& x, float& y);
    static CRGB getPinwheelColor(float x, float y, uint32_t time, const ColorPalette& palette);
};