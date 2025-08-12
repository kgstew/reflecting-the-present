#include "patterns.h"

void runSolidPattern(ChasePattern* pattern)
{
    pattern->last_update = current_time;

    // Use the first color in the palette for solid pattern
    CRGB solid_color = (pattern->palette_size > 0) ? pattern->palette[0] : CRGB::Black;

    // Apply solid color to all target strips
    for (uint8_t i = 0; i < pattern->num_target_strips; i++) {
        uint8_t strip_id = pattern->target_strips[i];
        if (strip_id >= 22)
            continue; // Invalid strip ID

        // Skip strips that are currently active in FlashBulb patterns
        if (isStripActiveInFlashBulb(strip_id)) {
            continue;
        }

        // Apply solid color across the strip
        for (uint16_t led = 0; led < strips[strip_id].length; led++) {
            CRGB final_color = solid_color;

            // Apply transition blending if transitioning
            if (pattern->is_transitioning) {
                unsigned long transition_elapsed = current_time - pattern->transition_start_time;

                if (transition_elapsed < pattern->transition_duration) {
                    if (!pattern->is_active) {
                        // Transitioning in (fade in from existing color to solid color)
                        uint8_t transition_blend = (transition_elapsed * 255) / pattern->transition_duration;
                        CRGB existing_color = getLED(strip_id, led);
                        final_color = existing_color.lerp8(final_color, transition_blend);
                    } else {
                        // Transitioning out (fade out to black)
                        uint8_t transition_blend = 255 - (transition_elapsed * 255) / pattern->transition_duration;
                        final_color.fadeToBlackBy(255 - transition_blend);
                    }
                }
            }

            getLED(strip_id, led) = final_color;
        }
    }
}