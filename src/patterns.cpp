#include "patterns.h"

PatternQueue pattern_queue = { .queue_size = 0, .queue_start_time = 0, .is_running = false };
FlashBulbManager flashbulb_manager = { .pattern_count = 0 };

void addPatternToQueue(CRGB* palette, uint8_t palette_size, uint8_t* target_strips, uint8_t num_target_strips,
    uint16_t speed, unsigned long transition_delay, uint16_t transition_duration)
{
    if (pattern_queue.queue_size >= MAX_QUEUE_SIZE)
        return;

    ChasePattern& pattern = pattern_queue.patterns[pattern_queue.queue_size];

    // Copy palette
    for (uint8_t i = 0; i < palette_size && i < MAX_PALETTE_SIZE; i++) {
        pattern.palette[i] = palette[i];
    }
    pattern.palette_size = palette_size;

    // Copy target strips
    for (uint8_t i = 0; i < num_target_strips && i < MAX_TARGET_STRIPS; i++) {
        pattern.target_strips[i] = target_strips[i];
    }
    pattern.num_target_strips = num_target_strips;

    pattern.speed = speed;
    pattern.transition_delay = transition_delay;
    pattern.last_update = 0;
    pattern.chase_position = 0;
    pattern.is_active = false;
    pattern.is_transitioning = false;
    pattern.transition_start_time = 0;
    pattern.transition_duration = transition_duration;

    pattern_queue.queue_size++;
}

void startPatternQueue()
{
    if (pattern_queue.queue_size > 0) {
        pattern_queue.queue_start_time = current_time;
        pattern_queue.is_running = true;

        // Initialize all patterns
        for (uint8_t i = 0; i < pattern_queue.queue_size; i++) {
            pattern_queue.patterns[i].is_active = false;
            pattern_queue.patterns[i].is_transitioning = false;
        }
    }
}

void stopPatternQueue() { pattern_queue.is_running = false; }

void clearPatternQueue()
{
    pattern_queue.queue_size = 0;
    pattern_queue.is_running = false;
}

void updatePatternQueue()
{
    if (!pattern_queue.is_running || pattern_queue.queue_size == 0)
        return;

    unsigned long elapsed_time = current_time - pattern_queue.queue_start_time;

    // Find the maximum transition delay to know when to loop
    unsigned long max_delay = 0;
    for (uint8_t i = 0; i < pattern_queue.queue_size; i++) {
        if (pattern_queue.patterns[i].transition_delay > max_delay) {
            max_delay = pattern_queue.patterns[i].transition_delay;
        }
    }

    // Add some time after the last pattern starts before looping (e.g., 5 seconds)
    unsigned long loop_time = max_delay + 5000;

    // Check each pattern to see if it should start based on its transition delay
    for (uint8_t i = 0; i < pattern_queue.queue_size; i++) {
        ChasePattern& pattern = pattern_queue.patterns[i];

        // Check if transition delay has elapsed and pattern isn't already active
        if (elapsed_time >= pattern.transition_delay && !pattern.is_active && !pattern.is_transitioning) {
            // Start transition for new pattern
            pattern.is_transitioning = true;
            pattern.transition_start_time = current_time;
            pattern.last_update = current_time;
            pattern.chase_position = 0;

            // Mark any existing patterns on these strips as transitioning out
            for (uint8_t j = 0; j < pattern_queue.queue_size; j++) {
                if (j != i && pattern_queue.patterns[j].is_active) {
                    ChasePattern& existing_pattern = pattern_queue.patterns[j];

                    // Check if patterns share any strips
                    bool shares_strips = false;
                    for (uint8_t k = 0; k < pattern.num_target_strips; k++) {
                        for (uint8_t l = 0; l < existing_pattern.num_target_strips; l++) {
                            if (pattern.target_strips[k] == existing_pattern.target_strips[l]) {
                                shares_strips = true;
                                break;
                            }
                        }
                        if (shares_strips)
                            break;
                    }

                    if (shares_strips && !existing_pattern.is_transitioning) {
                        existing_pattern.is_transitioning = true;
                        existing_pattern.transition_start_time = current_time;
                    }
                }
            }
        }

        // Update transition state
        if (pattern.is_transitioning) {
            unsigned long transition_elapsed = current_time - pattern.transition_start_time;
            if (transition_elapsed >= pattern.transition_duration) {
                // Transition complete
                pattern.is_transitioning = false;
                if (!pattern.is_active) {
                    pattern.is_active = true;
                }
            }
        }
    }

    // Loop the queue when enough time has passed
    if (elapsed_time >= loop_time) {
        // Reset the queue start time to create a loop
        pattern_queue.queue_start_time = current_time;

        // Reset all patterns to inactive so they can start again
        for (uint8_t i = 0; i < pattern_queue.queue_size; i++) {
            pattern_queue.patterns[i].is_active = false;
            pattern_queue.patterns[i].is_transitioning = false;
        }
    }
}

void runQueuedChasePattern()
{
    if (!pattern_queue.is_running || pattern_queue.queue_size == 0)
        return;

    updatePatternQueue();

    // Run all active or transitioning chase patterns FIRST (to set base pattern)
    bool any_pattern_updated = false;
    for (uint8_t i = 0; i < pattern_queue.queue_size; i++) {
        ChasePattern& pattern = pattern_queue.patterns[i];
        if (pattern.is_active || pattern.is_transitioning) {
            runChasePattern(&pattern);
            any_pattern_updated = true;
        }
    }
    
    // Update FlashBulb patterns LAST (they may override or blend with chase patterns)
    updateFlashBulbPatterns();
    
    // Check if any FlashBulb patterns are active
    for (uint8_t i = 0; i < flashbulb_manager.pattern_count; i++) {
        if (flashbulb_manager.patterns[i].state != FLASHBULB_INACTIVE) {
            any_pattern_updated = true;
            break;
        }
    }

    // Only call FastLED.show() once per frame if any patterns updated
    if (any_pattern_updated) {
        FastLED.show();
    }
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
                            if (&other_pattern != pattern && (other_pattern.is_active || other_pattern.is_transitioning)) {
                                // Check if this strip is in the other pattern
                                for (uint8_t s = 0; s < other_pattern.num_target_strips; s++) {
                                    if (other_pattern.target_strips[s] == strip_id) {
                                        strip_has_transition = true;
                                        break;
                                    }
                                }
                            }
                            if (strip_has_transition) break;
                        }
                        
                        if (strip_has_transition || !pattern->is_active) {
                            unsigned long transition_elapsed = current_time - pattern->transition_start_time;
                            
                            if (transition_elapsed < pattern->transition_duration) {
                                if (pattern->is_active) {
                                    // Transitioning out (fade out)
                                    uint8_t transition_blend = 255 - (transition_elapsed * 255) / pattern->transition_duration;
                                    blended_color.fadeToBlackBy(255 - transition_blend);
                                } else {
                                    // Transitioning in (fade in)
                                    uint8_t transition_blend = (transition_elapsed * 255) / pattern->transition_duration;
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

void setupPatternProgram()
{
    // Clear any existing patterns
    clearPatternQueue();

    // Define some palettes
    static CRGB warm_palette[] = { CRGB::Red, CRGB::Orange, CRGB::Yellow };
    static CRGB cool_palette[] = { CRGB::Blue, CRGB::Cyan, CRGB::Green };
    static CRGB rainbow_palette[] = { CRGB::Red, CRGB::Orange, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Purple };
    static CRGB sunset_palette[] = { CRGB::Purple, CRGB::Magenta, CRGB::Orange, CRGB::Red };

    // Define target strip groups
    static uint8_t all_strips[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 };
    static uint8_t outside[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
    static uint8_t inside[] = { 14, 15, 16, 17, 18, 19, 20, 21 };
    static uint8_t exterior_rings[] = { 0, 1, 2, 11, 12, 13 };

    // Add patterns to queue with different durations and speeds
    addPatternToQueue(warm_palette, 3, all_strips, 22, 50, 0); // 10 seconds - warm colors on all strips
    addPatternToQueue(sunset_palette, 4, exterior_rings, 6, 80, 6000); // 6 seconds - sunset colors on corners, slower
    addPatternToQueue(cool_palette, 3, outside, 14, 30, 8000); // 8 seconds - cool colors on first half, faster
    addPatternToQueue(warm_palette, 6, inside, 8, 40, 12000); // 12 seconds - rainbow on second half
    addPatternToQueue(rainbow_palette, 6, all_strips, 22, 25, 15000); // 15 seconds - fast rainbow on all strips

    // Start the pattern program
    startPatternQueue();
}

// FlashBulb pattern functions
void initFlashBulbManager() {
    flashbulb_manager.pattern_count = 0;
    for (uint8_t i = 0; i < MAX_FLASHBULB_PATTERNS; i++) {
        flashbulb_manager.patterns[i].state = FLASHBULB_INACTIVE;
        flashbulb_manager.patterns[i].num_target_strips = 0;
        flashbulb_manager.patterns[i].saved_color_count = 0;
    }
}

void addFlashBulbPattern(uint8_t* target_strips, uint8_t num_target_strips) {
    if (flashbulb_manager.pattern_count >= MAX_FLASHBULB_PATTERNS) return;
    
    FlashBulbPattern& pattern = flashbulb_manager.patterns[flashbulb_manager.pattern_count];
    
    // Copy target strips
    for (uint8_t i = 0; i < num_target_strips && i < MAX_TARGET_STRIPS; i++) {
        pattern.target_strips[i] = target_strips[i];
    }
    pattern.num_target_strips = num_target_strips;
    pattern.state = FLASHBULB_INACTIVE;
    pattern.saved_color_count = 0;
    
    flashbulb_manager.pattern_count++;
}

void triggerFlashBulb(uint8_t pattern_index) {
    if (pattern_index >= flashbulb_manager.pattern_count) return;
    
    FlashBulbPattern& pattern = flashbulb_manager.patterns[pattern_index];
    
    // Save current colors for transition back
    pattern.saved_color_count = 0;
    for (uint8_t i = 0; i < pattern.num_target_strips; i++) {
        uint8_t strip_id = pattern.target_strips[i];
        if (strip_id >= 22) continue;
        
        uint8_t pin_index = strip_map[strip_id];
        uint16_t strip_start_offset = 0;
        
        // Calculate the starting LED position for this strip within the pin's LED array
        for (uint8_t s = 0; s < strip_id; s++) {
            if (strip_map[s] == pin_index) {
                strip_start_offset += strip_lengths[s];
            }
        }
        
        uint16_t strip_length = strip_lengths[strip_id];
        
        // Save current colors
        for (uint16_t led = 0; led < strip_length && pattern.saved_color_count < (MAX_TARGET_STRIPS * 122); led++) {
            pattern.saved_colors[pattern.saved_color_count] = pin_configs[pin_index].led_array[strip_start_offset + led];
            pattern.saved_color_count++;
        }
    }
    
    // Set all target LEDs to white immediately
    for (uint8_t i = 0; i < pattern.num_target_strips; i++) {
        uint8_t strip_id = pattern.target_strips[i];
        if (strip_id >= 22) continue;
        
        uint8_t pin_index = strip_map[strip_id];
        uint16_t strip_start_offset = 0;
        
        // Calculate the starting LED position for this strip within the pin's LED array
        for (uint8_t s = 0; s < strip_id; s++) {
            if (strip_map[s] == pin_index) {
                strip_start_offset += strip_lengths[s];
            }
        }
        
        uint16_t strip_length = strip_lengths[strip_id];
        
        // Set to full white
        for (uint16_t led = 0; led < strip_length; led++) {
            pin_configs[pin_index].led_array[strip_start_offset + led] = CRGB::White;
        }
    }
    
    // Start the flash sequence
    pattern.state = FLASHBULB_FLASH;
    pattern.start_time = current_time;
    
    FastLED.show();
}

void updateFlashBulbPatterns() {
    for (uint8_t i = 0; i < flashbulb_manager.pattern_count; i++) {
        FlashBulbPattern& pattern = flashbulb_manager.patterns[i];
        
        if (pattern.state != FLASHBULB_INACTIVE) {
            runFlashBulbPattern(&pattern);
        }
    }
}

void runFlashBulbPattern(FlashBulbPattern* pattern) {
    unsigned long elapsed = current_time - pattern->start_time;
    
    switch (pattern->state) {
        case FLASHBULB_FLASH:
            // Flash is instantaneous, immediately start fade
            pattern->state = FLASHBULB_FADE_TO_BLACK;
            pattern->start_time = current_time;
            Serial.println("FlashBulb: Starting fade to black phase");
            break;
            
        case FLASHBULB_FADE_TO_BLACK:
            if (elapsed < 5000) { // 5 second fade to black
                uint8_t fade_amount = (elapsed * 255) / 5000;
                
                // Apply fade to all target strips
                for (uint8_t i = 0; i < pattern->num_target_strips; i++) {
                    uint8_t strip_id = pattern->target_strips[i];
                    if (strip_id >= 22) continue;
                    
                    uint8_t pin_index = strip_map[strip_id];
                    uint16_t strip_start_offset = 0;
                    
                    // Calculate the starting LED position for this strip within the pin's LED array
                    for (uint8_t s = 0; s < strip_id; s++) {
                        if (strip_map[s] == pin_index) {
                            strip_start_offset += strip_lengths[s];
                        }
                    }
                    
                    uint16_t strip_length = strip_lengths[strip_id];
                    
                    // Fade from white to black
                    for (uint16_t led = 0; led < strip_length; led++) {
                        CRGB white = CRGB::White;
                        CRGB black = CRGB::Black;
                        pin_configs[pin_index].led_array[strip_start_offset + led] = white.lerp8(black, fade_amount);
                    }
                }
                
                // Debug output every second
                static unsigned long last_debug = 0;
                if (current_time - last_debug >= 1000) {
                    last_debug = current_time;
                    Serial.print("FlashBulb fade progress: ");
                    Serial.print((elapsed * 100) / 5000);
                    Serial.println("%");
                }
            } else {
                // Fade complete, start transition back
                pattern->state = FLASHBULB_TRANSITION_BACK;
                pattern->start_time = current_time;
                
                // Ensure LEDs are black at the start of transition back phase
                for (uint8_t i = 0; i < pattern->num_target_strips; i++) {
                    uint8_t strip_id = pattern->target_strips[i];
                    if (strip_id >= 22) continue;
                    
                    uint8_t pin_index = strip_map[strip_id];
                    uint16_t strip_start_offset = 0;
                    
                    // Calculate the starting LED position for this strip within the pin's LED array
                    for (uint8_t s = 0; s < strip_id; s++) {
                        if (strip_map[s] == pin_index) {
                            strip_start_offset += strip_lengths[s];
                        }
                    }
                    
                    uint16_t strip_length = strip_lengths[strip_id];
                    
                    // Ensure black at transition start
                    for (uint16_t led = 0; led < strip_length; led++) {
                        pin_configs[pin_index].led_array[strip_start_offset + led] = CRGB::Black;
                    }
                }
                
                Serial.println("FlashBulb: Starting transition back to chase pattern");
            }
            break;
            
        case FLASHBULB_TRANSITION_BACK:
            if (elapsed < 2000) { // 2 second transition back
                uint8_t transition_amount = (elapsed * 255) / 2000;
                
                // During transition back, we blend from black to whatever the chase patterns 
                // have already set (since chase patterns run BEFORE FlashBulb in the main loop)
                for (uint8_t i = 0; i < pattern->num_target_strips; i++) {
                    uint8_t strip_id = pattern->target_strips[i];
                    if (strip_id >= 22) continue;
                    
                    uint8_t pin_index = strip_map[strip_id];
                    uint16_t strip_start_offset = 0;
                    
                    // Calculate the starting LED position for this strip within the pin's LED array
                    for (uint8_t s = 0; s < strip_id; s++) {
                        if (strip_map[s] == pin_index) {
                            strip_start_offset += strip_lengths[s];
                        }
                    }
                    
                    uint16_t strip_length = strip_lengths[strip_id];
                    
                    // Blend from black to the current chase pattern colors
                    for (uint16_t led = 0; led < strip_length; led++) {
                        // Get the current color that the chase pattern set
                        CRGB current_chase_color = pin_configs[pin_index].led_array[strip_start_offset + led];
                        CRGB black = CRGB::Black;
                        
                        // Blend from black to the current chase pattern color based on transition progress
                        pin_configs[pin_index].led_array[strip_start_offset + led] = black.lerp8(current_chase_color, transition_amount);
                    }
                }
                
                // Debug output
                static unsigned long last_debug2 = 0;
                if (current_time - last_debug2 >= 500) {
                    last_debug2 = current_time;
                    Serial.print("FlashBulb transition progress: ");
                    Serial.print((elapsed * 100) / 2000);
                    Serial.println("%");
                }
            } else {
                // Transition complete, return to inactive
                pattern->state = FLASHBULB_INACTIVE;
                Serial.println("FlashBulb: Effect complete, returning to normal patterns");
            }
            break;
            
        default:
            break;
    }
}