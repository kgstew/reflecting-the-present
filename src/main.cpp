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

unsigned long current_time;

void setup()
{
    Serial.begin(115200);
    Serial.print("Connecting");
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

    // Setup the pattern program
    setupPatternProgram();

    // Initialize FlashBulb system
    initFlashBulbManager();

    // Add FlashBulb patterns for demo
    uint8_t demo_strips_1[] = { 0, 1, 2 };
    addFlashBulbPattern(demo_strips_1, 3);

    uint8_t demo_strips_2[] = { 10, 11, 12, 13, 14 };
    addFlashBulbPattern(demo_strips_2, 5);

    uint8_t demo_strips_3[] = { 5, 15, 20 };
    addFlashBulbPattern(demo_strips_3, 3);
}

void demoFlashBulb()
{
    static unsigned long last_demo = 0;

    if (current_time - last_demo >= 10000) { // Every 5 seconds
        last_demo = current_time;

        // Random number of strips (0-5)
        uint8_t num_random_strips = random(0, 6);

        if (num_random_strips > 0) {
            // Create random strip array
            uint8_t random_strips[5];
            for (uint8_t i = 0; i < num_random_strips; i++) {
                random_strips[i] = random(0, 22); // Random strip 0-21
            }

            // Add temporary FlashBulb pattern
            addFlashBulbPattern(random_strips, num_random_strips);

            // Trigger the newly added pattern (it will be the last one)
            uint8_t pattern_index = flashbulb_manager.pattern_count - 1;
            triggerFlashBulb(pattern_index);

            Serial.print("FlashBulb demo triggered on ");
            Serial.print(num_random_strips);
            Serial.print(" strips: ");
            for (uint8_t i = 0; i < num_random_strips; i++) {
                Serial.print(random_strips[i]);
                if (i < num_random_strips - 1)
                    Serial.print(", ");
            }
            Serial.println();
        } else {
            Serial.println("FlashBulb demo: No strips selected this time");
        }
    }
}

void loop()
{
    current_time = millis();

    // Run demo FlashBulb trigger
    demoFlashBulb();

    // Run the queued pattern program
    runQueuedChasePattern();
}
