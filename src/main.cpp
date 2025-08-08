#include "patterns.h"
#include <Arduino.h>
#include <FastLED.h>

#define BRIGHTNESS 255
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define NUM_PINS 6

#define PIN1 5
#define PIN2 18
#define PIN3 19
#define PIN4 21
#define PIN5 22
#define PIN6 23

uint16_t strip_lengths[]
    = { 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122 };

CRGB pin1_leds[366];
CRGB pin2_leds[488];
CRGB pin3_leds[488];
CRGB pin4_leds[366];
CRGB pin5_leds[488];
CRGB pin6_leds[488];

PinConfig pin_configs[NUM_PINS]
    = { { PIN1, 3, 122, 366, pin1_leds }, { PIN2, 4, 122, 488, pin2_leds }, { PIN3, 4, 122, 488, pin3_leds },
          { PIN4, 3, 122, 366, pin4_leds }, { PIN5, 4, 122, 488, pin5_leds }, { PIN6, 4, 122, 488, pin6_leds } };

uint8_t strip_map[] = { 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5 };

void setup()
{
    Serial.begin(115200);
    delay(3000);

    FastLED.addLeds<LED_TYPE, PIN1, COLOR_ORDER>(pin1_leds, pin_configs[0].total_leds).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, PIN2, COLOR_ORDER>(pin2_leds, pin_configs[1].total_leds).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, PIN3, COLOR_ORDER>(pin3_leds, pin_configs[2].total_leds).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, PIN4, COLOR_ORDER>(pin4_leds, pin_configs[3].total_leds).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, PIN5, COLOR_ORDER>(pin5_leds, pin_configs[4].total_leds).setCorrection(TypicalLEDStrip);
    FastLED.addLeds<LED_TYPE, PIN6, COLOR_ORDER>(pin6_leds, pin_configs[5].total_leds).setCorrection(TypicalLEDStrip);

    FastLED.setBrightness(BRIGHTNESS);

    Serial.println("LED configuration:");
    uint16_t total_leds = 0;
    uint8_t strip_index = 0;

    for (int pin = 0; pin < NUM_PINS; pin++) {
        Serial.printf("Pin %d: %d LEDs (%d strips) - Strips: ", pin_configs[pin].pin, pin_configs[pin].total_leds,
            pin_configs[pin].num_strips);

        for (int s = 0; s < pin_configs[pin].num_strips; s++) {
            Serial.printf("%d", strip_lengths[strip_index]);
            if (s < pin_configs[pin].num_strips - 1)
                Serial.print(", ");
            strip_index++;
        }
        Serial.println();
        total_leds += pin_configs[pin].total_leds;
    }

    Serial.printf("Total: %d LEDs across %d strips\n", total_leds, strip_index);
    Serial.println("Expected: 3+4+4+3+4+4 = 22 strips total");

    // Initialize the strip pattern manager
    StripPatternManager::init(22); // Total of 22 strips
    Serial.println("Strip pattern manager initialized");
}

void loop()
{
    static uint32_t start_time = millis();
    static uint32_t last_demo_time = 0;
    uint32_t current_time = millis() - start_time;

    // Demo: trigger different patterns on different strips every 15 seconds
    uint8_t demo_step = (current_time / 15000) % 4;

    if (current_time - last_demo_time > 15000) {
        last_demo_time = current_time;

        switch (demo_step) {
        case 0: {
            ColorPalette none;
            none.size = 0;
            uint8_t outside[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
            StripPatternManager::setColorChasePattern(outside, 14, none, 80, 0); // Fast chase speed, infinite duration
            break;
        }
        case 1: {
            // Color chase with ocean colors
            Serial.println("Demo: Color chase with ocean palette");
            ColorPalette ocean_palette;
            ocean_palette.colors[0] = CRGB::Navy;
            ocean_palette.colors[1] = CRGB::Blue;
            ocean_palette.colors[2] = CRGB::DeepSkyBlue;
            ocean_palette.colors[3] = CRGB::Cyan;
            ocean_palette.colors[4] = CRGB::Aqua;
            ocean_palette.size = 5;

            uint8_t outside[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
            StripPatternManager::setColorChasePattern(
                outside, 14, ocean_palette, 80, 0); // Fast chase speed, infinite duration
            break;
        }

        case 2: {
            // Flash multiple strips with white bulb effect
            Serial.println("Demo: Multiple strips flashbulb white");
            uint8_t flashbulb_strips[] = { 0, 3, 7, 11, 14, 18 }; // Mix of vertical and horizontal strips
            StripPatternManager::setFlashBulbPattern(flashbulb_strips, 6);
            break;
        }

        case 3: {
            // Demo pinwheel pattern with warm colors
            Serial.println("Demo: Pinwheel pattern with warm colors");
            ColorPalette warm_palette;
            warm_palette.colors[0] = CRGB::Red;
            warm_palette.colors[1] = CRGB::Orange;
            warm_palette.colors[2] = CRGB::Yellow;
            warm_palette.colors[3] = CRGB::Pink;
            warm_palette.size = 4;

            uint8_t pinwheel_strips[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
            StripPatternManager::setPinwheelPattern(pinwheel_strips, 14, warm_palette, 0); // Infinite duration
            break;
        }

            // case 2:
            //     // Turn strips 10-12 red for 2 seconds
            //     Serial.println("Demo: Strips 10-12 solid red");
            //     StripPatternManager::setStripPattern(10, PATTERN_SOLID_COLOR, CRGB::Red, 2000);
            //     StripPatternManager::setStripPattern(11, PATTERN_SOLID_COLOR, CRGB::Red, 2000);
            //     StripPatternManager::setStripPattern(12, PATTERN_SOLID_COLOR, CRGB::Red, 2000);
            //     break;

            // case 3:
            //     // Turn strip 0 off for 2 seconds
            //     Serial.println("Demo: Strip 0 off");
            //     StripPatternManager::setStripPattern(0, PATTERN_OFF, CRGB::Black, 2000);
            //     break;

            // case 4:
            //     // Sequential blue pattern with delays
            //     Serial.println("Demo: Sequential blue pattern");
            //     for (int i = 15; i < 19; i++) {
            //         StripPatternManager::setStripPatternWithDelay(i, PATTERN_SOLID_COLOR, CRGB::Blue, (i-15)*200,
            //         1500);
            //     }
            //     break;

            // case 5:
            //     // Mixed colors on pin 6 strips (strips 19-21)
            //     Serial.println("Demo: Mixed colors on pin 6");
            //     StripPatternManager::setStripPattern(19, PATTERN_SOLID_COLOR, CRGB::Green, 2500);
            //     StripPatternManager::setStripPattern(20, PATTERN_SOLID_COLOR, CRGB::Purple, 2500);
            //     StripPatternManager::setStripPattern(21, PATTERN_SOLID_COLOR, CRGB::Yellow, 2500);
            //     break;
        }

        last_demo_time = current_time;
    }

    // Update all strips with their individual patterns
    StripPatternManager::updateAllStrips(pin_configs, strip_lengths, NUM_PINS, current_time);

    FastLED.show();
    delay(10);
}