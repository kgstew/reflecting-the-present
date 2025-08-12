#include "patterns.h"

void runChasePattern(ChasePattern* pattern)
{
    // Update FastLED palette if pattern has changed
    updatePatternPalette(pattern);

    // Continue with chase pattern logic for PATTERN_CHASE
    unsigned long speed_delay = convertSpeedToDelay(pattern->speed);
    if (current_time - pattern->last_update >= speed_delay) {
        pattern->last_update = current_time;

        // Apply continuous chase pattern across all target strips
        uint16_t global_led_position = 0;
        const uint16_t STRIP_OFFSET = 10; // Internal offset between strips for better visual separation

        for (uint8_t i = 0; i < pattern->num_target_strips; i++) {
            uint8_t strip_id = pattern->target_strips[i];
            if (strip_id >= 22)
                continue; // Invalid strip ID

            // Skip strips that are currently active in FlashBulb patterns
            if (isStripActiveInFlashBulb(strip_id)) {
                // Still need to advance global_led_position for proper chase continuity
                global_led_position += strips[strip_id].length + STRIP_OFFSET;
                continue;
            }

            CRGBSet strip_set = getStripSet(strip_id);
            uint16_t strip_length = getStripLength(strip_id);

            // Apply chase pattern using FastLED's ColorFromPalette for smooth blending
            for (uint16_t led = 0; led < strip_length; led++) {
                // Calculate palette index based on position in the chase with strip offset
                uint8_t palette_index
                    = ((global_led_position + pattern->chase_position) * 255) / (pattern->palette_size * 10);

                // Use FastLED's ColorFromPalette for smooth color transitions
                CRGB blended_color = ColorFromPalette(pattern->fastled_palette, palette_index, 255, LINEARBLEND);

                // Apply transition blending only if this strip is actually being changed
                if (pattern->is_transitioning) {
                    // Check if there's an active pattern on this strip that's different from current pattern
                    bool strip_has_transition = false;

                    // Check if this strip is shared with other active patterns
                    for (uint8_t p = 0; p < pattern_queue.queue_size; p++) {
                        ChasePattern& other_pattern = pattern_queue.patterns[p];
                        if (&other_pattern != pattern && (other_pattern.is_active || other_pattern.is_transitioning)) {
                            // Check if this strip is in the other pattern
                            for (uint8_t s = 0; s < other_pattern.num_target_strips; s++) {
                                if (other_pattern.target_strips[s] == strip_id) {
                                    strip_has_transition = true;
                                    break;
                                }
                            }
                        }
                        if (strip_has_transition)
                            break;
                    }

                    if (strip_has_transition || !pattern->is_active) {
                        unsigned long transition_elapsed = current_time - pattern->transition_start_time;

                        if (transition_elapsed < pattern->transition_duration) {
                            if (pattern->is_active) {
                                // Transitioning out (fade out) - use FastLED's fadeToBlackBy
                                uint8_t fade_amount = (transition_elapsed * 255) / pattern->transition_duration;
                                blended_color.fadeToBlackBy(fade_amount);
                            } else {
                                // Transitioning in (fade in) - use FastLED's lerp8
                                uint8_t transition_blend = (transition_elapsed * 255) / pattern->transition_duration;
                                CRGB existing_color = getStripLED(strip_id, led);
                                blended_color = existing_color.lerp8(blended_color, transition_blend);
                            }
                        }
                    }
                }

                getStripLED(strip_id, led) = blended_color;
                global_led_position++;
            }

            // Add strip offset for better visual separation between strips
            global_led_position += STRIP_OFFSET;
        }

        // Advance chase position - advance more positions for higher speeds to match rainbow pattern timing
        uint8_t advance_step = (pattern->speed / 10) + 1; // Speed 1-10 = 1 step, 11-20 = 2 steps, etc.
        pattern->chase_position = (pattern->chase_position + advance_step) % (pattern->palette_size * 10);
    }
}