#include "patterns.h"

#define SPEED_MULTIPLIER 5

void runRainbowHorizontalPattern(ChasePattern* pattern)
{
    unsigned long speed_delay = convertSpeedToDelay(pattern->speed);
    if (current_time - pattern->last_update >= speed_delay) {
        pattern->last_update = current_time;

        // Calculate hue shift based on time and speed
        // Higher speed = faster rainbow cycling, so divide by (101 - speed) to invert the relationship
        uint8_t speed_divisor = (101 - pattern->speed) * SPEED_MULTIPLIER;
        uint8_t hue_offset = (current_time / speed_divisor) % 256;
        
        // Apply rainbow pattern horizontally across strips
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
            
            // Calculate hue for this strip based on its position in the group
            // Rainbow moves across strips (perpendicular to strip direction)
            uint8_t strip_hue = hue_offset + (i * 255 / pattern->num_target_strips);
            
            // Fill entire strip with the same hue (solid color per strip)
            CHSV hsv_color(strip_hue, 255, 255);
            CRGB rgb_color;
            hsv2rgb_rainbow(hsv_color, rgb_color);
            
            strip_set.fill_solid(rgb_color);
            
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