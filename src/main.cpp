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

struct PinConfig {
    uint8_t pin;
    uint8_t num_strips;
    uint16_t leds_per_strip;
    uint16_t total_leds;
    CRGB* led_array;
};

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

void chasePattern(uint8_t* target_strips, uint8_t num_target_strips, CRGB* palette, uint8_t palette_size, uint16_t speed) {
    static unsigned long last_update = 0;
    static uint16_t chase_position = 0;
    
    if (current_time - last_update >= speed) {
        last_update = current_time;
        
        // Calculate total length of all target strips
        uint16_t total_pattern_length = 0;
        for (uint8_t i = 0; i < num_target_strips; i++) {
            uint8_t strip_id = target_strips[i];
            if (strip_id < 22) {
                total_pattern_length += strip_lengths[strip_id];
            }
        }
        
        // Apply continuous chase pattern across all target strips
        uint16_t global_led_position = 0;
        for (uint8_t i = 0; i < num_target_strips; i++) {
            uint8_t strip_id = target_strips[i];
            if (strip_id >= 22) continue; // Invalid strip ID
            
            uint8_t pin_index = strip_map[strip_id];
            uint16_t strip_start_offset = 0;
            
            // Calculate the starting LED position for this strip within the pin's LED array
            for (uint8_t s = 0; s < strip_id; s++) {
                if (strip_map[s] == pin_index) {
                    strip_start_offset += strip_lengths[s];
                }
            }
            
            uint16_t strip_length = strip_lengths[strip_id];
            
            // Apply chase pattern with blending across the strip
            for (uint16_t led = 0; led < strip_length; led++) {
                uint16_t pattern_position = (global_led_position + chase_position) % (palette_size * 10);
                uint8_t color_index = pattern_position / 10;
                uint8_t blend_amount = (pattern_position % 10) * 25; // 0-250 blend amount
                
                if (color_index < palette_size) {
                    CRGB current_color = palette[color_index];
                    CRGB next_color = palette[(color_index + 1) % palette_size];
                    
                    // Blend between current and next color
                    CRGB blended_color = current_color.lerp8(next_color, blend_amount);
                    pin_configs[pin_index].led_array[strip_start_offset + led] = blended_color;
                }
                
                global_led_position++;
            }
        }
        
        // Advance chase position
        chase_position = (chase_position + 1) % (palette_size * 10);
        
        FastLED.show();
    }
}

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
}

void loop() {
    current_time = millis();
    
    // Example usage of chase pattern
    static CRGB palette[] = {CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::Yellow};
    static uint8_t target_strips[] = {0, 1, 2, 3}; // First 4 strips
    
    chasePattern(target_strips, 4, palette, 4, 50);
}