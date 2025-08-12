#include "patterns.h"

void runBreathingPattern(ChasePattern* pattern)
{
    pattern->last_update = current_time;
    
    // Use FastLED's beatsin8 for smooth breathing effect
    uint8_t breath = beatsin8(pattern->speed, 50, 255);  // Breathe between 50-255 brightness
    
    // Cycle through palette colors at 1/4 the speed of breathing
    CRGB base_color = CRGB::White;
    if (pattern->palette_size > 0) {
        uint8_t color_index = beatsin8(pattern->speed / 4, 0, pattern->palette_size - 1);
        base_color = pattern->palette[color_index];
    }
    
    // Apply breathing pattern to all target strips
    for (uint8_t i = 0; i < pattern->num_target_strips; i++) {
        uint8_t strip_id = pattern->target_strips[i];
        if (strip_id >= 22)
            continue;

        // Skip strips that are currently active in FlashBulb patterns
        if (isStripActiveInFlashBulb(strip_id)) {
            continue;
        }

        CRGBSet strip_set = getStripSet(strip_id);
        uint16_t strip_length = getStripLength(strip_id);
        
        // Fill with base color and apply breathing brightness
        strip_set.fill_solid(base_color);
        strip_set.fadeToBlackBy(255 - breath);
        
        // Apply transition blending if transitioning
        if (pattern->is_transitioning) {
            unsigned long transition_elapsed = current_time - pattern->transition_start_time;
            if (transition_elapsed < pattern->transition_duration) {
                if (!pattern->is_active) {
                    // Transitioning in
                    uint8_t transition_blend = (transition_elapsed * 255) / pattern->transition_duration;
                    for (uint16_t led = 0; led < strip_length; led++) {
                        CRGB existing_color = getStripLED(strip_id, led);
                        getStripLED(strip_id, led) = existing_color.lerp8(getStripLED(strip_id, led), transition_blend);
                    }
                } else {
                    // Transitioning out
                    uint8_t fade_amount = (transition_elapsed * 255) / pattern->transition_duration;
                    strip_set.fadeToBlackBy(fade_amount);
                }
            }
        }
    }
}