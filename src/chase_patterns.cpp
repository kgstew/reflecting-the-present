#include "patterns.h"

bool isStripActiveInFlashBulb(uint8_t strip_id)
{
    for (uint8_t i = 0; i < flashbulb_manager.pattern_count; i++) {
        FlashBulbPattern& flashbulb = flashbulb_manager.patterns[i];
        // Only block chase patterns during FLASH and FADE_TO_BLACK phases
        // Allow chase patterns during TRANSITION_BACK so we have colors to blend to
        if (flashbulb.state == FLASHBULB_FLASH || flashbulb.state == FLASHBULB_FADE_TO_BLACK) {
            // Check if this strip is in the active FlashBulb pattern
            for (uint8_t j = 0; j < flashbulb.num_target_strips; j++) {
                if (flashbulb.target_strips[j] == strip_id) {
                    return true;
                }
            }
        }
    }
    return false;
}

void runChasePattern(ChasePattern* pattern)
{
    if (current_time - pattern->last_update >= pattern->speed) {
        pattern->last_update = current_time;

        // Apply continuous chase pattern across all target strips
        uint16_t global_led_position = 0;
        for (uint8_t i = 0; i < pattern->num_target_strips; i++) {
            uint8_t strip_id = pattern->target_strips[i];
            if (strip_id >= 22)
                continue; // Invalid strip ID

            // Skip strips that are currently active in FlashBulb patterns
            if (isStripActiveInFlashBulb(strip_id)) {
                // Still need to advance global_led_position for proper chase continuity
                global_led_position += strip_lengths[strip_id];
                continue;
            }

            uint8_t pin_index = strip_map[strip_id];
            uint16_t strip_start_offset = 0;

            // Calculate the starting LED position for this strip within the pin's LED array
            for (uint8_t s = 0; s < strip_id; s++) {
                if (strip_map[s] == pin_index) {
                    strip_start_offset += strip_lengths[s];
                }
            }

            uint16_t strip_length = strip_lengths[strip_id];

            // Apply chase pattern with blending across the strip
            for (uint16_t led = 0; led < strip_length; led++) {
                uint16_t pattern_position
                    = (global_led_position + pattern->chase_position) % (pattern->palette_size * 10);
                uint8_t color_index = pattern_position / 10;
                uint8_t blend_amount = (pattern_position % 10) * 25; // 0-250 blend amount

                if (color_index < pattern->palette_size) {
                    CRGB current_color = pattern->palette[color_index];
                    CRGB next_color = pattern->palette[(color_index + 1) % pattern->palette_size];

                    // Blend between current and next color in palette
                    CRGB blended_color = current_color.lerp8(next_color, blend_amount);

                    // Apply transition blending only if this strip is actually being changed
                    if (pattern->is_transitioning) {
                        // Check if there's an active pattern on this strip that's different from current pattern
                        bool strip_has_transition = false;

                        // Check if this strip is shared with other active patterns
                        for (uint8_t p = 0; p < pattern_queue.queue_size; p++) {
                            ChasePattern& other_pattern = pattern_queue.patterns[p];
                            if (&other_pattern != pattern
                                && (other_pattern.is_active || other_pattern.is_transitioning)) {
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
                                    // Transitioning out (fade out)
                                    uint8_t transition_blend
                                        = 255 - (transition_elapsed * 255) / pattern->transition_duration;
                                    blended_color.fadeToBlackBy(255 - transition_blend);
                                } else {
                                    // Transitioning in (fade in)
                                    uint8_t transition_blend
                                        = (transition_elapsed * 255) / pattern->transition_duration;
                                    CRGB existing_color = pin_configs[pin_index].led_array[strip_start_offset + led];
                                    blended_color = existing_color.lerp8(blended_color, transition_blend);
                                }
                            }
                        }
                    }

                    pin_configs[pin_index].led_array[strip_start_offset + led] = blended_color;
                }

                global_led_position++;
            }
        }

        // Advance chase position
        pattern->chase_position = (pattern->chase_position + 1) % (pattern->palette_size * 10);
    }
}

void chasePattern(
    uint8_t* target_strips, uint8_t num_target_strips, CRGB* palette, uint8_t palette_size, uint16_t speed)
{
    static unsigned long last_update = 0;
    static uint16_t chase_position = 0;

    if (current_time - last_update >= speed) {
        last_update = current_time;

        // Apply continuous chase pattern across all target strips
        uint16_t global_led_position = 0;
        for (uint8_t i = 0; i < num_target_strips; i++) {
            uint8_t strip_id = target_strips[i];
            if (strip_id >= 22)
                continue; // Invalid strip ID

            uint8_t pin_index = strip_map[strip_id];
            uint16_t strip_start_offset = 0;

            // Calculate the starting LED position for this strip within the pin's LED array
            for (uint8_t s = 0; s < strip_id; s++) {
                if (strip_map[s] == pin_index) {
                    strip_start_offset += strip_lengths[s];
                }
            }

            uint16_t strip_length = strip_lengths[strip_id];

            // Apply chase pattern with blending across the strip
            for (uint16_t led = 0; led < strip_length; led++) {
                uint16_t pattern_position = (global_led_position + chase_position) % (palette_size * 10);
                uint8_t color_index = pattern_position / 10;
                uint8_t blend_amount = (pattern_position % 10) * 25; // 0-250 blend amount

                if (color_index < palette_size) {
                    CRGB current_color = palette[color_index];
                    CRGB next_color = palette[(color_index + 1) % palette_size];

                    // Blend between current and next color
                    CRGB blended_color = current_color.lerp8(next_color, blend_amount);
                    pin_configs[pin_index].led_array[strip_start_offset + led] = blended_color;
                }

                global_led_position++;
            }
        }

        // Advance chase position
        chase_position = (chase_position + 1) % (palette_size * 10);

        FastLED.show();
    }
}