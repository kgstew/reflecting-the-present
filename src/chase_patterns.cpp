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
    // Update FastLED palette if pattern has changed
    updatePatternPalette(pattern);
    
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
                global_led_position += strips[strip_id].length;
                continue;
            }

            CRGBSet strip_set = getStripSet(strip_id);
            uint16_t strip_length = getStripLength(strip_id);

            // Apply chase pattern using FastLED's ColorFromPalette for smooth blending
            for (uint16_t led = 0; led < strip_length; led++) {
                // Calculate palette index based on position in the chase
                uint8_t palette_index = ((global_led_position + pattern->chase_position) * 255) / (pattern->palette_size * 10);
                
                // Use FastLED's ColorFromPalette for smooth color transitions
                CRGB blended_color = ColorFromPalette(pattern->fastled_palette, palette_index, 255, LINEARBLEND);

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
        }

        // Advance chase position
        pattern->chase_position = (pattern->chase_position + 1) % (pattern->palette_size * 10);
    }
}

void chasePattern(uint8_t* target_strips, uint8_t num_target_strips, CRGB* palette, uint8_t palette_size, uint8_t speed)
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

            CRGBSet strip_set = getStripSet(strip_id);
            uint16_t strip_length = getStripLength(strip_id);

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

                    getStripLED(strip_id, led) = blended_color;
                }

                global_led_position++;
            }
        }

        // Advance chase position
        chase_position = (chase_position + 1) % (palette_size * 10);

        FastLED.show();
    }
}