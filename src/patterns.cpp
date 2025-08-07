#include "patterns.h"

void PatternManager::rainbowChase(PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t num_pins, uint32_t time_ms) {
    const uint8_t chase_pins = 4;
    const uint16_t chase_speed = 30;
    
    uint16_t total_chase_leds = 0;
    for (int pin = 0; pin < chase_pins; pin++) {
        total_chase_leds += pin_configs[pin].total_leds;
    }
    
    float chase_offset = (time_ms / (float)chase_speed);
    
    uint16_t global_led_index = 0;
    uint8_t strip_index = 0;
    
    for (int pin = 0; pin < chase_pins; pin++) {
        uint16_t led_position = 0;
        
        for (int strip = 0; strip < pin_configs[pin].num_strips; strip++) {
            uint16_t strip_length = strip_lengths[strip_index];
            
            for (int led = 0; led < strip_length; led++) {
                float hue_position = (global_led_index + chase_offset) * 360.0 / 256.0;
                uint8_t hue = ((uint16_t)(hue_position + (time_ms / 50)) % 360) * 255 / 360;
                
                pin_configs[pin].led_array[led_position] = CHSV(hue, 255, 255);
                
                led_position++;
                global_led_index++;
            }
            strip_index++;
        }
    }
    
    for (int pin = chase_pins; pin < num_pins; pin++) {
        for (int i = 0; i < pin_configs[pin].total_leds; i++) {
            pin_configs[pin].led_array[i] = CRGB::Black;
        }
    }
}

void PatternManager::whiteChase(PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t num_pins, uint32_t time_ms) {
    clearAllLEDs(pin_configs, num_pins);
    
    const uint16_t chase_length = 10;
    const uint16_t chase_speed = 40;
    
    uint16_t total_leds = getTotalLEDCount(pin_configs, strip_lengths, num_pins);
    
    uint16_t chase_position = (time_ms / chase_speed) % total_leds;
    
    uint16_t global_led_index = 0;
    uint8_t strip_index = 0;
    
    for (int pin = 0; pin < num_pins; pin++) {
        uint16_t led_position = 0;
        
        for (int strip = 0; strip < pin_configs[pin].num_strips; strip++) {
            uint16_t strip_length = strip_lengths[strip_index];
            
            for (int led = 0; led < strip_length; led++) {
                bool in_chase = false;
                
                for (int i = 0; i < chase_length; i++) {
                    uint16_t chase_led = (chase_position + i) % total_leds;
                    if (global_led_index == chase_led) {
                        in_chase = true;
                        break;
                    }
                }
                
                if (in_chase) {
                    pin_configs[pin].led_array[led_position] = CRGB::White;
                } else {
                    pin_configs[pin].led_array[led_position] = CRGB::Black;
                }
                
                led_position++;
                global_led_index++;
            }
            strip_index++;
        }
    }
}

void PatternManager::solidPin(PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t num_pins, uint8_t target_pin, uint8_t num_strips_to_light) {
    clearAllLEDs(pin_configs, num_pins);
    
    if (target_pin >= num_pins) {
        return;
    }
    
    uint8_t strips_to_light = min(num_strips_to_light, pin_configs[target_pin].num_strips);
    
    uint8_t strip_index = 0;
    for (int pin = 0; pin < target_pin; pin++) {
        strip_index += pin_configs[pin].num_strips;
    }
    
    uint16_t led_position = 0;
    
    for (int strip = 0; strip < strips_to_light; strip++) {
        uint16_t strip_length = strip_lengths[strip_index + strip];
        
        for (int led = 0; led < strip_length; led++) {
            pin_configs[target_pin].led_array[led_position] = CRGB::White;
            led_position++;
        }
    }
}

void PatternManager::clearAllLEDs(PinConfig* pin_configs, uint8_t num_pins) {
    for (int pin = 0; pin < num_pins; pin++) {
        for (int i = 0; i < pin_configs[pin].total_leds; i++) {
            pin_configs[pin].led_array[i] = CRGB::Black;
        }
    }
}

uint16_t PatternManager::getGlobalLEDIndex(PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t target_pin, uint8_t target_strip, uint8_t target_led) {
    uint16_t global_index = 0;
    uint8_t strip_index = 0;
    
    for (int pin = 0; pin < target_pin; pin++) {
        global_index += pin_configs[pin].total_leds;
        strip_index += pin_configs[pin].num_strips;
    }
    
    for (int strip = 0; strip < target_strip; strip++) {
        global_index += strip_lengths[strip_index];
        strip_index++;
    }
    
    global_index += target_led;
    return global_index;
}

uint16_t PatternManager::getTotalLEDCount(PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t num_pins) {
    uint16_t total = 0;
    for (int pin = 0; pin < num_pins; pin++) {
        total += pin_configs[pin].total_leds;
    }
    return total;
}