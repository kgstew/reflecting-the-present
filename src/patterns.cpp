#include "patterns.h"
#include <math.h>

// StripPatternManager static variables
StripState* StripPatternManager::strip_states = nullptr;
uint8_t StripPatternManager::total_strip_count = 0;
uint32_t StripPatternManager::global_rainbow_time = 0;

// StripPatternManager implementation
void StripPatternManager::init(uint8_t total_strips)
{
    total_strip_count = total_strips;
    if (strip_states != nullptr) {
        delete[] strip_states;
    }
    strip_states = new StripState[total_strips];

    // Initialize all strips to rainbow chase by default
    for (int i = 0; i < total_strips; i++) {
        strip_states[i].pattern = PATTERN_RAINBOW_CHASE;
        strip_states[i].start_time = 0;
        strip_states[i].duration = 0; // infinite
        strip_states[i].color = CRGB::White;
        strip_states[i].active = true;
        strip_states[i].reverse = false; // default to forward direction
        strip_states[i].palette.size = 0; // no palette by default
        strip_states[i].pinwheel_group_id = 255; // no group by default
    }
}

void StripPatternManager::setStripPattern(uint8_t strip_id, PatternType pattern, CRGB color, uint32_t duration)
{
    if (strip_id >= total_strip_count)
        return;

    strip_states[strip_id].pattern = pattern;
    strip_states[strip_id].start_time = millis();
    strip_states[strip_id].duration = duration;
    strip_states[strip_id].color = color;
    strip_states[strip_id].active = true;
}

void StripPatternManager::setStripPatternWithDelay(
    uint8_t strip_id, PatternType pattern, CRGB color, uint32_t delay_ms, uint32_t duration)
{
    if (strip_id >= total_strip_count)
        return;

    strip_states[strip_id].pattern = pattern;
    strip_states[strip_id].start_time = millis() + delay_ms;
    strip_states[strip_id].duration = duration;
    strip_states[strip_id].color = color;
    strip_states[strip_id].active = true;
}

void StripPatternManager::setPinwheelPattern(uint8_t* strip_ids, uint8_t num_strips, ColorPalette palette, uint32_t duration)
{
    static uint8_t next_group_id = 0;
    uint8_t group_id = next_group_id++;
    
    uint32_t current_millis = millis();
    
    for (int i = 0; i < num_strips; i++) {
        uint8_t strip_id = strip_ids[i];
        if (strip_id >= total_strip_count)
            continue;
            
        strip_states[strip_id].pattern = PATTERN_PINWHEEL;
        strip_states[strip_id].start_time = current_millis;
        strip_states[strip_id].duration = duration;
        strip_states[strip_id].active = true;
        strip_states[strip_id].palette = palette;
        strip_states[strip_id].pinwheel_group_id = group_id;
    }
}

void StripPatternManager::updateAllStrips(
    PinConfig* pin_configs, uint16_t* strip_lengths, uint8_t num_pins, uint32_t current_time)
{
    global_rainbow_time = current_time;

    uint8_t strip_id = 0;

    for (int pin = 0; pin < num_pins; pin++) {
        uint16_t led_offset = 0;

        for (int strip_on_pin = 0; strip_on_pin < pin_configs[pin].num_strips; strip_on_pin++) {
            uint16_t strip_length = strip_lengths[strip_id];

            // Check if pattern duration has expired
            if (strip_states[strip_id].duration > 0
                && current_time > strip_states[strip_id].start_time + strip_states[strip_id].duration) {
                // Reset to default rainbow chase
                strip_states[strip_id].pattern = PATTERN_RAINBOW_CHASE;
                strip_states[strip_id].start_time = current_time;
                strip_states[strip_id].duration = 0;
                strip_states[strip_id].active = true;
            }

            renderStrip(
                strip_id, pin, strip_on_pin, strip_length, pin_configs[pin].led_array, led_offset, current_time);

            led_offset += strip_length;
            strip_id++;
        }
    }
}

void StripPatternManager::renderStrip(uint8_t strip_id, uint8_t pin, uint8_t strip_on_pin, uint16_t strip_length,
    CRGB* led_array, uint16_t led_offset, uint32_t current_time)
{
    if (strip_id >= total_strip_count || !strip_states[strip_id].active)
        return;

    // Don't render if pattern hasn't started yet
    if (current_time < strip_states[strip_id].start_time) {
        return;
    }

    uint32_t pattern_time = current_time - strip_states[strip_id].start_time;

    switch (strip_states[strip_id].pattern) {
    case PATTERN_RAINBOW_CHASE: {
        const uint16_t chase_speed = 30;
        float chase_offset = (global_rainbow_time / (float)chase_speed);

        // Calculate global LED position for this strip
        uint16_t global_led_base = strip_id * 122; // Simple approximation for now

        for (int led = 0; led < strip_length; led++) {
            uint16_t pattern_led = transformLedIndex(strip_id, led, strip_length);
            float hue_position = ((global_led_base + pattern_led) + chase_offset) * 360.0 / 256.0;
            uint8_t hue = ((uint16_t)(hue_position + (global_rainbow_time / 50)) % 360) * 255 / 360;
            led_array[led_offset + led] = CHSV(hue, 255, 255);
        }
        break;
    }

    case PATTERN_SOLID_COLOR:
        for (int i = 0; i < strip_length; i++) {
            led_array[led_offset + i] = strip_states[strip_id].color;
        }
        break;

    case PATTERN_OFF:
        for (int i = 0; i < strip_length; i++) {
            led_array[led_offset + i] = CRGB::Black;
        }
        break;

    case PATTERN_FLASHBULB: {
        const uint32_t FLASH_DURATION = 5000;
        const uint32_t FADE_DURATION = 2000;
        const uint32_t TOTAL_DURATION = FLASH_DURATION + FADE_DURATION;

        if (pattern_time <= FLASH_DURATION) {
            float fade_progress = (float)pattern_time / FLASH_DURATION;
            uint8_t brightness = 255 - (uint8_t)(fade_progress * (255 - 51));

            for (int i = 0; i < strip_length; i++) {
                led_array[led_offset + i] = CRGB(brightness, brightness, brightness);
            }
        } else {
            uint32_t fade_time = pattern_time - FLASH_DURATION;
            float fade_progress = (float)fade_time / FADE_DURATION;

            if (fade_progress >= 1.0) {
                fade_progress = 1.0;
                strip_states[strip_id].pattern = PATTERN_RAINBOW_CHASE;
                strip_states[strip_id].start_time = current_time;
                strip_states[strip_id].duration = 0;
            }

            uint16_t global_led_base = strip_id * 122;
            const uint16_t chase_speed = 30;
            float chase_offset = (global_rainbow_time / (float)chase_speed);

            for (int led = 0; led < strip_length; led++) {
                float hue_position = ((global_led_base + led) + chase_offset) * 360.0 / 256.0;
                uint8_t hue = ((uint16_t)(hue_position + (global_rainbow_time / 50)) % 360) * 255 / 360;

                uint8_t target_brightness = (uint8_t)(204 * fade_progress);
                uint16_t transformed_led = transformLedIndex(strip_id, led, strip_length);
                led_array[led_offset + transformed_led] = CHSV(hue, 255, 51 + target_brightness);
            }
        }
        break;
    }

    case PATTERN_PINWHEEL: {
        for (int led = 0; led < strip_length; led++) {
            float x, y;
            getLedMatrixPosition(strip_id, led, x, y);
            CRGB color = getPinwheelColor(x, y, pattern_time, strip_states[strip_id].palette);
            uint16_t pattern_led = transformLedIndex(strip_id, led, strip_length);
            led_array[led_offset + pattern_led] = color;
        }
        break;
    }

    default:
        break;
    }
}

void StripPatternManager::clearStrip(uint8_t strip_id)
{
    if (strip_id >= total_strip_count)
        return;
    strip_states[strip_id].pattern = PATTERN_OFF;
    strip_states[strip_id].active = true;
}

void StripPatternManager::clearAllStrips()
{
    for (int i = 0; i < total_strip_count; i++) {
        strip_states[i].pattern = PATTERN_OFF;
        strip_states[i].active = true;
    }
}

StripState* StripPatternManager::getStripState(uint8_t strip_id)
{
    if (strip_id >= total_strip_count)
        return nullptr;
    return &strip_states[strip_id];
}

uint8_t StripPatternManager::getStripId(uint8_t pin, uint8_t strip_on_pin)
{
    // This needs to be implemented based on your pin configuration
    // For now, simple linear mapping
    return pin * 4 + strip_on_pin; // Assumes max 4 strips per pin
}

void StripPatternManager::getStripPosition(uint8_t strip_id, uint8_t& pin, uint8_t& strip_on_pin)
{
    // Simple reverse mapping
    pin = strip_id / 4;
    strip_on_pin = strip_id % 4;
}

void StripPatternManager::setStripReverse(uint8_t strip_id, bool reverse)
{
    if (strip_id >= total_strip_count)
        return;
    strip_states[strip_id].reverse = reverse;
}

uint16_t StripPatternManager::transformLedIndex(uint8_t strip_id, uint16_t led_index, uint16_t strip_length)
{
    if (strip_id >= total_strip_count || !strip_states[strip_id].reverse)
        return led_index;

    return strip_length - 1 - led_index;
}

void StripPatternManager::getStripCoordinates(uint8_t strip_id, float& center_x, float& center_y)
{
    // Map strip positions based on the layout
    // Strips 0-2: Pin 1 (left column, vertical)
    // Strips 3-6: Pin 2 (left-center, vertical)  
    // Strips 7-10: Pin 3 (right-center, vertical)
    // Strips 11-13: Pin 4 (right column, vertical)
    // Strips 14-17: Pin 5 (horizontal, top)
    // Strips 18-21: Pin 6 (horizontal, bottom)
    
    if (strip_id <= 2) {
        // Pin 1: left vertical strips
        center_x = 0.0f;
        center_y = (strip_id * 40.0f) + 20.0f;
    } else if (strip_id <= 6) {
        // Pin 2: left-center vertical strips  
        center_x = 30.0f;
        center_y = ((strip_id - 3) * 30.0f) + 15.0f;
    } else if (strip_id <= 10) {
        // Pin 3: right-center vertical strips
        center_x = 60.0f;
        center_y = ((strip_id - 7) * 30.0f) + 15.0f;
    } else if (strip_id <= 13) {
        // Pin 4: right vertical strips
        center_x = 90.0f;
        center_y = ((strip_id - 11) * 40.0f) + 20.0f;
    } else if (strip_id <= 17) {
        // Pin 5: top horizontal strips
        center_x = ((strip_id - 14) * 30.0f) + 15.0f;
        center_y = -10.0f;
    } else if (strip_id <= 21) {
        // Pin 6: bottom horizontal strips
        center_x = ((strip_id - 18) * 30.0f) + 15.0f;
        center_y = 130.0f;
    }
}

void StripPatternManager::getLedMatrixPosition(uint8_t strip_id, uint16_t led_index, float& x, float& y)
{
    float center_x, center_y;
    getStripCoordinates(strip_id, center_x, center_y);
    
    // Calculate LED position based on strip orientation
    if (strip_id <= 13) {
        // Vertical strips (pins 1-4)
        x = center_x;
        y = center_y + (led_index - 61) * 1.0f; // Center around middle of 122 LEDs
    } else {
        // Horizontal strips (pins 5-6)
        x = center_x + (led_index - 61) * 1.0f; // Center around middle of 122 LEDs
        y = center_y;
    }
}

CRGB StripPatternManager::getPinwheelColor(float x, float y, uint32_t time, const ColorPalette& palette)
{
    if (palette.size == 0) {
        return CRGB::Black;
    }
    
    // Calculate center of the matrix (roughly)
    float center_x = 45.0f;
    float center_y = 60.0f;
    
    // Calculate angle from center
    float dx = x - center_x;
    float dy = y - center_y;
    float angle = atan2(dy, dx);
    
    // Add time-based rotation
    float rotation_speed = 0.002f; // adjust for desired speed
    angle += time * rotation_speed;
    
    // Calculate distance for radial effect
    float distance = sqrt(dx * dx + dy * dy);
    
    // Map angle to palette index
    float normalized_angle = (angle + PI) / (2 * PI); // 0 to 1
    normalized_angle = fmod(normalized_angle + (distance * 0.01f), 1.0f); // Add distance-based variation
    
    uint8_t palette_index = (uint8_t)(normalized_angle * palette.size);
    palette_index = palette_index % palette.size;
    
    // Blend between adjacent colors for smoother transitions
    uint8_t next_index = (palette_index + 1) % palette.size;
    float blend = (normalized_angle * palette.size) - palette_index;
    
    CRGB color1 = palette.colors[palette_index];
    CRGB color2 = palette.colors[next_index];
    
    // Linear interpolation between colors
    uint8_t r = color1.r + (blend * (color2.r - color1.r));
    uint8_t g = color1.g + (blend * (color2.g - color1.g));
    uint8_t b = color1.b + (blend * (color2.b - color1.b));
    
    return CRGB(r, g, b);
}