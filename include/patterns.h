#ifndef PATTERNS_H
#define PATTERNS_H

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
    PATTERN_CUSTOM
};

struct StripState {
    PatternType pattern;
    uint32_t start_time;
    uint32_t duration;  // 0 = infinite
    CRGB color;
    bool active;
};

class StripPatternManager {
public:
    static void init(uint8_t total_strips);
    static void setStripPattern(uint8_t strip_id, PatternType pattern, CRGB color = CRGB::White, uint32_t duration = 0);
    static void setStripPatternWithDelay(uint8_t strip_id, PatternType pattern, CRGB color, uint32_t delay_ms, uint32_t duration = 0);
    static void updateAllStrips(PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t num_pins, uint32_t current_time);
    static void clearStrip(uint8_t strip_id);
    static void clearAllStrips();
    static StripState* getStripState(uint8_t strip_id);
    
private:
    static StripState* strip_states;
    static uint8_t total_strip_count;
    static uint32_t global_rainbow_time;
    
    static void renderStrip(uint8_t strip_id, uint8_t pin, uint8_t strip_on_pin, uint16_t strip_length, CRGB* led_array, uint16_t led_offset, uint32_t current_time);
    static uint8_t getStripId(uint8_t pin, uint8_t strip_on_pin);
    static void getStripPosition(uint8_t strip_id, uint8_t& pin, uint8_t& strip_on_pin);
};

// Legacy patterns for backward compatibility
class PatternManager {
public:
    static void rainbowChase(PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t num_pins, uint32_t time_ms);
    static void whiteChase(PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t num_pins, uint32_t time_ms);
    static void solidPin(PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t num_pins, uint8_t target_pin, uint8_t num_strips_to_light);
    static void clearAllLEDs(PinConfig* pin_configs, uint8_t num_pins);
    
private:
    static uint16_t getGlobalLEDIndex(PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t target_pin, uint8_t target_strip, uint8_t target_led);
    static uint16_t getTotalLEDCount(PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t num_pins);
};

#endif