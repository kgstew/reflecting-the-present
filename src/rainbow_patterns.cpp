#include "patterns.h"

void runRainbowPattern(ChasePattern* pattern)
{
    unsigned long speed_delay = convertSpeedToDelay(pattern->speed);
    if (current_time - pattern->last_update >= speed_delay) {
        pattern->last_update = current_time;

        // Apply rainbow pattern to all target strips using FastLED's HSV
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
            
            // Use FastLED's fill_rainbow for smooth rainbow effect
            uint8_t start_hue = (current_time / (pattern->speed * 10)) % 256;  // Rotating rainbow
            fill_rainbow(strip_set, strip_length, start_hue, 255 / strip_length);
            
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
        
        pattern->chase_position = (pattern->chase_position + 1) % 256;
    }
}