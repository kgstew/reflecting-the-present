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

uint16_t strip_lengths[] = {122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122, 122};

CRGB pin1_leds[366];
CRGB pin2_leds[488];
CRGB pin3_leds[488];
CRGB pin4_leds[366];
CRGB pin5_leds[488];
CRGB pin6_leds[488];

PinConfig pin_configs[NUM_PINS] = {
    {PIN1, 3, 122, 366, pin1_leds},
    {PIN2, 4, 122, 488, pin2_leds},
    {PIN3, 4, 122, 488, pin3_leds},
    {PIN4, 3, 122, 366, pin4_leds},
    {PIN5, 4, 122, 488, pin5_leds},
    {PIN6, 4, 122, 488, pin6_leds}
};

uint8_t strip_map[] = {0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};

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
        Serial.printf("Pin %d: %d LEDs (%d strips) - Strips: ", 
                     pin_configs[pin].pin, 
                     pin_configs[pin].total_leds, 
                     pin_configs[pin].num_strips);
        
        for (int s = 0; s < pin_configs[pin].num_strips; s++) {
            Serial.printf("%d", strip_lengths[strip_index]);
            if (s < pin_configs[pin].num_strips - 1) Serial.print(", ");
            strip_index++;
        }
        Serial.println();
        total_leds += pin_configs[pin].total_leds;
    }
    
    Serial.printf("Total: %d LEDs across %d strips\n", total_leds, strip_index);
    Serial.println("Expected: 3+4+4+3+4+4 = 22 strips total");
}

void loop()
{
    static uint8_t hue = 0;
    uint8_t global_strip_index = 0;
    
    for (int pin = 0; pin < NUM_PINS; pin++) {
        uint8_t pinOffset = pin * 40;
        uint16_t led_position = 0;
        
        for (int strip = 0; strip < pin_configs[pin].num_strips; strip++) {
            uint16_t strip_length = strip_lengths[global_strip_index];
            uint8_t stripOffset = strip * 60;
            
            if (led_position + strip_length > pin_configs[pin].total_leds) {
                Serial.printf("ERROR: Pin %d would overflow! led_position=%d, strip_length=%d, total_leds=%d\n", 
                             pin, led_position, strip_length, pin_configs[pin].total_leds);
                break;
            }
            
            for (int led = 0; led < strip_length; led++) {
                pin_configs[pin].led_array[led_position] = CHSV(
                    hue + pinOffset + stripOffset + (led * 2), 
                    255, 
                    255
                );
                led_position++;
            }
            global_strip_index++;
        }
    }

    FastLED.show();
    EVERY_N_MILLISECONDS(20) { hue++; }
}