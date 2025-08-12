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

            CRGBSet strip_set = getStripSet(strip_id);
            uint16_t strip_length_actual = getStripLength(strip_id);
            
            // Start with black strip using FastLED's fill_solid
            strip_set.fill_solid(CRGB::Black);

            // Only apply white LEDs to the currently active strip
            if (i == current_strip_index && position_in_strip < strip_length) {
                // Calculate the chase window bounds
                uint16_t chase_start = position_in_strip;
                uint16_t chase_end = min((uint16_t)(position_in_strip + SINGLE_CHASE_LENGTH), strip_length_actual);
                
                // Fill the chase window with white using FastLED's subset
                if (chase_start < strip_length_actual && chase_end > chase_start) {
                    CRGBSet chase_window = strip_set(chase_start, chase_end - 1);
                    chase_window.fill_solid(CRGB::White);
                }
            }

            // Apply transition blending if transitioning
            if (pattern->is_transitioning) {
                unsigned long transition_elapsed = current_time - pattern->transition_start_time;

                if (transition_elapsed < pattern->transition_duration) {
                    if (!pattern->is_active) {
                        // Transitioning in (fade in from existing color to chase pattern)
                        uint8_t transition_blend = (transition_elapsed * 255) / pattern->transition_duration;
                        
                        // Use FastLED's lerp8 for smooth transitions
                        for (uint16_t led = 0; led < strip_length_actual; led++) {
                            CRGB existing_color = getStripLED(strip_id, led);
                            CRGB target_color = (i == current_strip_index && 
                                               led >= position_in_strip && 
                                               led < position_in_strip + SINGLE_CHASE_LENGTH &&
                                               position_in_strip < strip_length) ? CRGB::White : CRGB::Black;
                            getStripLED(strip_id, led) = existing_color.lerp8(target_color, transition_blend);
                        }
                    } else {
                        // Transitioning out (fade out to black)
                        uint8_t fade_amount = (transition_elapsed * 255) / pattern->transition_duration;
                        strip_set.fadeToBlackBy(fade_amount);
                    }
                }
            }
        }

        // Advance chase position - cycle through all strips
        uint16_t total_pattern_cycle = pattern->num_target_strips * total_chase_cycle;
        pattern->chase_position = (pattern->chase_position + 1) % total_pattern_cycle;
    }
}