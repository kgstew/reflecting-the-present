#include "patterns.h"

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
        // Keep previous pattern or default to off
        for (int i = 0; i < strip_length; i++) {
            led_array[led_offset + i] = CRGB::Black;
        }
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
            float hue_position = ((global_led_base + led) + chase_offset) * 360.0 / 256.0;
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