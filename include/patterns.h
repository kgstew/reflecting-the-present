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

class PatternManager {
public:
    static void rainbowChase(PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t num_pins, uint32_t time_ms);
    static void clearAllLEDs(PinConfig* pin_configs, uint8_t num_pins);
    
private:
    static uint16_t getGlobalLEDIndex(PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t target_pin, uint8_t target_strip, uint8_t target_led);
    static uint16_t getTotalLEDCount(PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t num_pins);
};

#endif