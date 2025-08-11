#include "patterns.h"

#define SINGLE_CHASE_LENGTH 10

void runSingleChasePattern(ChasePattern* pattern)
{
    unsigned long speed_delay = convertSpeedToDelay(pattern->speed);
    if (current_time - pattern->last_update >= speed_delay) {
        pattern->last_update = current_time;

        // Calculate which strip is currently active and position within that strip
        uint16_t strip_length = 122; // Assuming all strips are 122 LEDs
        uint16_t total_chase_cycle = strip_length + SINGLE_CHASE_LENGTH; // Length + gap (132 total)
        uint16_t current_strip_index = (pattern->chase_position / total_chase_cycle) % pattern->num_target_strips;
        uint16_t position_in_strip = pattern->chase_position % total_chase_cycle;

        // First, set all target strips to black
        for (uint8_t i = 0; i < pattern->num_target_strips; i++) {
            uint8_t strip_id = pattern->target_strips[i];
            if (strip_id >= 22)
                continue; // Invalid strip ID

            // Skip strips that are currently active in FlashBulb patterns
            if (isStripActiveInFlashBulb(strip_id)) {
                continue;
            }

            uint8_t pin_index = strip_map[strip_id];
            uint16_t strip_start_offset = strip_offsets[strip_id]; // Use pre-calculated offset

            // Set entire strip to black
            for (uint16_t led = 0; led < strip_lengths[strip_id]; led++) {
                CRGB final_color = CRGB::Black;

                // Only apply white LEDs to the currently active strip
                if (i == current_strip_index && position_in_strip < strip_length) {
                    // Check if this LED is within the 10-LED chase window
                    if (led >= position_in_strip && led < position_in_strip + SINGLE_CHASE_LENGTH && led < strip_lengths[strip_id]) {
                        final_color = CRGB::White;
                    }
                }

                // Apply transition blending if transitioning
                if (pattern->is_transitioning) {
                    unsigned long transition_elapsed = current_time - pattern->transition_start_time;

                    if (transition_elapsed < pattern->transition_duration) {
                        if (!pattern->is_active) {
                            // Transitioning in (fade in from existing color to chase pattern)
                            uint8_t transition_blend = (transition_elapsed * 255) / pattern->transition_duration;
                            uint16_t directional_led = getDirectionalLedIndex(strip_id, led);
                            CRGB existing_color = pin_configs[pin_index].led_array[strip_start_offset + directional_led];
                            final_color = existing_color.lerp8(final_color, transition_blend);
                        } else {
                            // Transitioning out (fade out to black)
                            uint8_t transition_blend = 255 - (transition_elapsed * 255) / pattern->transition_duration;
                            final_color.fadeToBlackBy(255 - transition_blend);
                        }
                    }
                }

                // Get the directional LED index (supports forward/reverse)
                uint16_t directional_led = getDirectionalLedIndex(strip_id, led);
                
                pin_configs[pin_index].led_array[strip_start_offset + directional_led] = final_color;
            }
        }

        // Advance chase position - cycle through all strips
        uint16_t total_pattern_cycle = pattern->num_target_strips * total_chase_cycle;
        pattern->chase_position = (pattern->chase_position + 1) % total_pattern_cycle;
    }
}