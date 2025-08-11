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

void runChasePatternLogic(ChasePattern* pattern)
{
    // Continue with chase pattern logic for PATTERN_CHASE
    unsigned long speed_delay = convertSpeedToDelay(pattern->speed);
    if (current_time - pattern->last_update >= speed_delay) {
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
            uint16_t strip_start_offset = strip_offsets[strip_id]; // Use pre-calculated offset

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

                    // Apply cached transition blending (calculated once per frame)
                    if (pattern_queue.strip_transitions[strip_id].has_transition) {
                        uint8_t transition_blend = pattern_queue.strip_transitions[strip_id].transition_blend;
                        if (pattern_queue.strip_transitions[strip_id].is_fading_in) {
                            // Transitioning in (fade in)
                            CRGB existing_color = pin_configs[pin_index].led_array[strip_start_offset + led];
                            blended_color = existing_color.lerp8(blended_color, transition_blend);
                        } else {
                            // Transitioning out (fade out)
                            blended_color.fadeToBlackBy(255 - transition_blend);
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
    uint8_t* target_strips, uint8_t num_target_strips, CRGB* palette, uint8_t palette_size, uint8_t speed)
{
    static unsigned long last_update = 0;
    static uint16_t chase_position = 0;

    unsigned long speed_delay = convertSpeedToDelay(speed);
    if (current_time - last_update >= speed_delay) {
        last_update = current_time;

        // Apply continuous chase pattern across all target strips
        uint16_t global_led_position = 0;
        for (uint8_t i = 0; i < num_target_strips; i++) {
            uint8_t strip_id = target_strips[i];
            if (strip_id >= 22)
                continue; // Invalid strip ID

            uint8_t pin_index = strip_map[strip_id];
            uint16_t strip_start_offset = strip_offsets[strip_id]; // Use pre-calculated offset

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