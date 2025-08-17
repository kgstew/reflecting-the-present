#include "patterns.h"

void runWarpPattern(ChasePattern* pattern)
{
    // Get warp parameters
    uint16_t acceleration_delay_seconds = pattern->params.warp.acceleration_delay;
    bool fade_previous = pattern->params.warp.fade_previous;
    
    // Convert acceleration delay to milliseconds
    unsigned long acceleration_delay = acceleration_delay_seconds * 1000;
    
    // Calculate current speed based on acceleration
    unsigned long pattern_elapsed = current_time - (pattern_queue.queue_start_time + pattern->transition_delay);
    float speed_factor = 1.0f;
    
    if (acceleration_delay > 0 && pattern_elapsed < acceleration_delay) {
        // Accelerate from initial speed to full speed over acceleration_delay
        speed_factor = (float)pattern_elapsed / (float)acceleration_delay;
        if (speed_factor < 0.1f) speed_factor = 0.1f; // Minimum 10% speed
    }
    
    // Calculate actual delay based on speed and acceleration
    unsigned long base_delay = convertSpeedToDelay(pattern->speed);
    unsigned long actual_delay = (unsigned long)(base_delay / speed_factor);
    
    if (current_time - pattern->last_update >= actual_delay) {
        pattern->last_update = current_time;
        
        // Determine current active strip index
        uint8_t current_strip_index = pattern->chase_position % pattern->num_target_strips;
        
        // Clear all strips first
        for (uint8_t i = 0; i < pattern->num_target_strips; i++) {
            uint8_t strip_id = pattern->target_strips[i];
            if (strip_id >= 22) continue;
            
            // Skip strips that are currently active in FlashBulb patterns
            if (isStripActiveInFlashBulb(strip_id)) continue;
            
            CRGBSet strip_set = getStripSet(strip_id);
            
            if (i == current_strip_index) {
                // This is the active strip - show the palette
                uint16_t strip_length = getStripLength(strip_id);
                
                if (pattern->palette_size > 0) {
                    // Fill the strip with palette colors
                    for (uint16_t led = 0; led < strip_length; led++) {
                        // Map LED position to palette color
                        uint8_t palette_index = (led * pattern->palette_size) / strip_length;
                        if (palette_index >= pattern->palette_size) {
                            palette_index = pattern->palette_size - 1;
                        }
                        
                        getStripLED(strip_id, led) = pattern->palette[palette_index];
                    }
                } else {
                    // Fallback to white if no palette
                    strip_set.fill_solid(CRGB::White);
                }
            } else if (fade_previous && i == ((current_strip_index - 1 + pattern->num_target_strips) % pattern->num_target_strips)) {
                // This is the previous strip - fade it out
                strip_set.fadeToBlackBy(200); // Fast fade
            } else {
                // All other strips are off
                strip_set.fill_solid(CRGB::Black);
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
        
        // Advance to next strip
        pattern->chase_position++;
        
        // Reset position if we've gone through all strips
        if (pattern->chase_position >= pattern->num_target_strips) {
            pattern->chase_position = 0;
        }
    }
}