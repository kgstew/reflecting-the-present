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
        
        // Clear all strips first
        for (uint8_t i = 0; i < pattern->num_target_strips; i++) {
            uint8_t strip_id = pattern->target_strips[i];
            if (strip_id >= 22) continue;
            
            if (isStripActiveInFlashBulb(strip_id)) continue;
            
            CRGBSet strip_set = getStripSet(strip_id);
            strip_set.fill_solid(CRGB::Black);
        }
        
        // Create pinwheel effect with rotating spokes
        uint8_t num_spokes = pattern->palette_size;
        if (num_spokes == 0) num_spokes = 4;
        
        for (uint8_t spoke = 0; spoke < num_spokes; spoke++) {
            // Calculate the angle for this spoke
            uint16_t spoke_angle = (rotation_angle + (spoke * 360 / num_spokes)) % 360;
            float angle_rad = spoke_angle * PI / 180.0;
            
            // Get spoke color
            CRGB spoke_color = (spoke < pattern->palette_size) ? 
                pattern->palette[spoke] : pattern->palette[0];
            
            // Draw line from center outward for this spoke
            float max_radius = sqrt(center_x * center_x + center_y * center_y);
            
            for (float radius = 0; radius <= max_radius; radius += 0.5) {
                // Calculate position in LED matrix
                float x = center_x + radius * cos(angle_rad);
                float y = center_y + radius * sin(angle_rad);
                
                // Convert to LED coordinates
                int led_pos = (int)(x + 0.5); // Round to nearest LED
                int strip_idx = (int)(y + 0.5); // Round to nearest strip
                
                // Check bounds
                if (led_pos >= 0 && led_pos < matrix_width && 
                    strip_idx >= 0 && strip_idx < matrix_height) {
                    
                    uint8_t strip_id = pattern->target_strips[strip_idx];
                    if (strip_id >= 22) continue;
                    
                    if (isStripActiveInFlashBulb(strip_id)) continue;
                    
                    // Calculate distance-based brightness fade
                    float distance_from_center = sqrt((x - center_x) * (x - center_x) + 
                                                     (y - center_y) * (y - center_y));
                    uint8_t brightness = 255 - (uint8_t)(distance_from_center * 200 / max_radius);
                    
                    // Apply minimum brightness to keep spokes visible
                    if (brightness < 50) brightness = 50;
                    
                    CRGB faded_color = spoke_color;
                    faded_color.fadeToBlackBy(255 - brightness);
                    
                    // Set the LED color, blending with existing if there's overlap
                    CRGB existing = getStripLED(strip_id, led_pos);
                    getStripLED(strip_id, led_pos) = existing + faded_color;
                }
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