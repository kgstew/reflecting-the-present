#include "patterns.h"

void runPinwheelPattern(ChasePattern* pattern)
{
    unsigned long speed_delay = convertSpeedToDelay(pattern->speed);
    if (current_time - pattern->last_update >= speed_delay) {
        pattern->last_update = current_time;

        // Calculate rotation angle based on time and speed
        // Higher speed = faster rotation, so divide by (101 - speed) to invert the relationship
        uint8_t speed_divisor = (101 - pattern->speed) * 5;
        uint16_t rotation_angle = (current_time / speed_divisor) % 360;
        
        // For inside strips: 8 strips of 122 LEDs each, arranged side by side
        // Matrix dimensions: 122 LEDs wide (x-axis) by 8 strips tall (y-axis)
        uint16_t matrix_width = 122;  // LEDs along each strip
        uint8_t matrix_height = pattern->num_target_strips;  // Number of strips
        
        // Find center of the LED matrix
        float center_x = (matrix_width - 1) / 2.0;  // Center LED position (around LED 61)
        float center_y = (matrix_height - 1) / 2.0; // Center strip (around strip 3.5 for 8 strips)
        
        // Fill all LEDs with interpolated colors based on angle from center
        for (uint8_t strip_idx = 0; strip_idx < pattern->num_target_strips; strip_idx++) {
            uint8_t strip_id = pattern->target_strips[strip_idx];
            if (strip_id >= 22) continue;
            
            if (isStripActiveInFlashBulb(strip_id)) continue;
            
            uint16_t strip_length = getStripLength(strip_id);
            
            for (uint16_t led_pos = 0; led_pos < strip_length; led_pos++) {
                // Calculate this LED's position relative to center
                float x = led_pos - center_x;
                float y = strip_idx - center_y;
                
                // Calculate angle from center to this LED
                float angle_to_led = atan2(y, x) * 180.0 / PI;
                if (angle_to_led < 0) angle_to_led += 360; // Convert to 0-360 range
                
                // Apply rotation offset
                float rotated_angle = fmod(angle_to_led - rotation_angle + 360, 360);
                
                // Scale the angle mapping to spread colors more evenly
                // Multiply by a factor to create more color cycles across the matrix
                float scaled_angle = fmod(rotated_angle * 3.0, 360.0); // 3x more color cycles
                
                // Map scaled angle to palette position for smooth interpolation
                float palette_position = scaled_angle * pattern->palette_size / 360.0;
                
                // Get the two adjacent palette colors to interpolate between
                uint8_t color_index1 = (uint8_t)palette_position % pattern->palette_size;
                uint8_t color_index2 = (color_index1 + 1) % pattern->palette_size;
                
                // Calculate interpolation amount
                float blend_factor = palette_position - floor(palette_position);
                uint8_t blend_amount = (uint8_t)(blend_factor * 255);
                
                // Interpolate between the two colors
                CRGB color1 = pattern->palette[color_index1];
                CRGB color2 = pattern->palette[color_index2];
                CRGB interpolated_color = color1.lerp8(color2, blend_amount);
                
                // Optional: Apply distance-based brightness fade for depth effect
                float distance_from_center = sqrt(x * x + y * y);
                float max_distance = sqrt(center_x * center_x + center_y * center_y);
                uint8_t brightness = 255 - (uint8_t)(distance_from_center * 100 / max_distance);
                if (brightness < 150) brightness = 150; // Keep a minimum brightness
                
                interpolated_color.fadeToBlackBy(255 - brightness);
                
                // Set the LED color
                getStripLED(strip_id, led_pos) = interpolated_color;
            }
        }
        
        // Apply transition blending if transitioning
        if (pattern->is_transitioning) {
            unsigned long transition_elapsed = current_time - pattern->transition_start_time;
            if (transition_elapsed < pattern->transition_duration) {
                for (uint8_t i = 0; i < pattern->num_target_strips; i++) {
                    uint8_t strip_id = pattern->target_strips[i];
                    if (strip_id >= 22) continue;
                    if (isStripActiveInFlashBulb(strip_id)) continue;
                    
                    CRGBSet strip_set = getStripSet(strip_id);
                    uint16_t strip_length = getStripLength(strip_id);
                    
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
        
        pattern->chase_position = (pattern->chase_position + 1) % 360;
    }
}