#include "patterns.h"

PatternQueue pattern_queue = { .queue_size = 0, .queue_start_time = 0, .is_running = false };


CRGB& getLED(uint8_t strip_id, uint16_t led_index)
{
    // Direct LED access using new unified configuration
    const StripConfig& strip = strips[strip_id];
    
    // Apply direction logic
    uint16_t actual_index = strip.reverse_direction ? 
        (strip.length - 1 - led_index) : led_index;
    
    // Return reference to the LED in the pin's array
    return strip.led_array_ptr[strip.start_offset + actual_index];
}

void configureStripDirections()
{
    // Configure strip directions from the configuration array
    extern bool strip_reverse_config[22];
    
    Serial.println("Configuring strip directions...");
    
    for (uint8_t i = 0; i < 22; i++) {
        strips[i].reverse_direction = strip_reverse_config[i];
        
        Serial.printf("Strip %d: %s\n", i, 
                     strips[i].reverse_direction ? "REVERSED" : "FORWARD");
    }
}

void initializeStripConfigs()
{
    // Validation and setup for the new strip configuration system
    Serial.println("Initializing unified strip configuration...");
    
    // Initialize strip configuration data using individual field assignment
    // Pin 1 (13) - 3 strips (strips 0-2)
    strips[0].physical_pin = 13; strips[0].pin_index = 0; strips[0].start_offset = 0; strips[0].length = 122; strips[0].reverse_direction = false; strips[0].led_array_ptr = pin1_leds;
    strips[1].physical_pin = 13; strips[1].pin_index = 0; strips[1].start_offset = 122; strips[1].length = 122; strips[1].reverse_direction = false; strips[1].led_array_ptr = pin1_leds;
    strips[2].physical_pin = 13; strips[2].pin_index = 0; strips[2].start_offset = 244; strips[2].length = 122; strips[2].reverse_direction = false; strips[2].led_array_ptr = pin1_leds;
    
    // Pin 2 (5) - 4 strips (strips 3-6)
    strips[3].physical_pin = 5; strips[3].pin_index = 1; strips[3].start_offset = 0; strips[3].length = 122; strips[3].reverse_direction = false; strips[3].led_array_ptr = pin2_leds;
    strips[4].physical_pin = 5; strips[4].pin_index = 1; strips[4].start_offset = 122; strips[4].length = 122; strips[4].reverse_direction = false; strips[4].led_array_ptr = pin2_leds;
    strips[5].physical_pin = 5; strips[5].pin_index = 1; strips[5].start_offset = 244; strips[5].length = 122; strips[5].reverse_direction = false; strips[5].led_array_ptr = pin2_leds;
    strips[6].physical_pin = 5; strips[6].pin_index = 1; strips[6].start_offset = 366; strips[6].length = 122; strips[6].reverse_direction = false; strips[6].led_array_ptr = pin2_leds;
    
    // Pin 3 (19) - 4 strips (strips 7-10)
    strips[7].physical_pin = 19; strips[7].pin_index = 2; strips[7].start_offset = 0; strips[7].length = 122; strips[7].reverse_direction = false; strips[7].led_array_ptr = pin3_leds;
    strips[8].physical_pin = 19; strips[8].pin_index = 2; strips[8].start_offset = 122; strips[8].length = 122; strips[8].reverse_direction = false; strips[8].led_array_ptr = pin3_leds;
    strips[9].physical_pin = 19; strips[9].pin_index = 2; strips[9].start_offset = 244; strips[9].length = 122; strips[9].reverse_direction = false; strips[9].led_array_ptr = pin3_leds;
    strips[10].physical_pin = 19; strips[10].pin_index = 2; strips[10].start_offset = 366; strips[10].length = 122; strips[10].reverse_direction = false; strips[10].led_array_ptr = pin3_leds;
    
    // Pin 4 (23) - 3 strips (strips 11-13)
    strips[11].physical_pin = 23; strips[11].pin_index = 3; strips[11].start_offset = 0; strips[11].length = 122; strips[11].reverse_direction = false; strips[11].led_array_ptr = pin4_leds;
    strips[12].physical_pin = 23; strips[12].pin_index = 3; strips[12].start_offset = 122; strips[12].length = 122; strips[12].reverse_direction = false; strips[12].led_array_ptr = pin4_leds;
    strips[13].physical_pin = 23; strips[13].pin_index = 3; strips[13].start_offset = 244; strips[13].length = 122; strips[13].reverse_direction = false; strips[13].led_array_ptr = pin4_leds;
    
    // Pin 5 (18) - 4 strips (strips 14-17)
    strips[14].physical_pin = 18; strips[14].pin_index = 4; strips[14].start_offset = 0; strips[14].length = 122; strips[14].reverse_direction = false; strips[14].led_array_ptr = pin5_leds;
    strips[15].physical_pin = 18; strips[15].pin_index = 4; strips[15].start_offset = 122; strips[15].length = 122; strips[15].reverse_direction = false; strips[15].led_array_ptr = pin5_leds;
    strips[16].physical_pin = 18; strips[16].pin_index = 4; strips[16].start_offset = 244; strips[16].length = 122; strips[16].reverse_direction = false; strips[16].led_array_ptr = pin5_leds;
    strips[17].physical_pin = 18; strips[17].pin_index = 4; strips[17].start_offset = 366; strips[17].length = 122; strips[17].reverse_direction = false; strips[17].led_array_ptr = pin5_leds;
    
    // Pin 6 (12) - 4 strips (strips 18-21)
    strips[18].physical_pin = 12; strips[18].pin_index = 5; strips[18].start_offset = 0; strips[18].length = 122; strips[18].reverse_direction = false; strips[18].led_array_ptr = pin6_leds;
    strips[19].physical_pin = 12; strips[19].pin_index = 5; strips[19].start_offset = 122; strips[19].length = 122; strips[19].reverse_direction = false; strips[19].led_array_ptr = pin6_leds;
    strips[20].physical_pin = 12; strips[20].pin_index = 5; strips[20].start_offset = 244; strips[20].length = 122; strips[20].reverse_direction = false; strips[20].led_array_ptr = pin6_leds;
    strips[21].physical_pin = 12; strips[21].pin_index = 5; strips[21].start_offset = 366; strips[21].length = 122; strips[21].reverse_direction = false; strips[21].led_array_ptr = pin6_leds;
    
    // Apply direction configuration
    configureStripDirections();
    
    // Initialize FastLED sets for each strip
    for (uint8_t i = 0; i < 22; i++) {
        StripConfig& strip = strips[i];
        
        // Verify pin index is valid
        if (strip.pin_index > 5) {
            Serial.printf("ERROR: Strip %d has invalid pin_index %d\n", i, strip.pin_index);
            continue;
        }
        
        // Verify the pin matches PinConfig
        if (strip.physical_pin != pin_configs[strip.pin_index].pin) {
            Serial.printf("WARNING: Strip %d pin mismatch - strip:%d vs pinconfig:%d\n", 
                         i, strip.physical_pin, pin_configs[strip.pin_index].pin);
        }
        
        // Verify LED array pointer matches
        if (strip.led_array_ptr != pin_configs[strip.pin_index].led_array) {
            Serial.printf("WARNING: Strip %d LED array pointer mismatch\n", i);
        }
        
        // Note: CRGBSets will be created on-demand in getStripSet() function
        
        Serial.printf("Strip %d: Pin %d, Offset %d, Length %d, Direction: %s\n", 
                     i, strip.physical_pin, strip.start_offset, strip.length,
                     strip.reverse_direction ? "REVERSED" : "FORWARD");
    }
    
    // Add debug output to verify addressing for problematic strips
    Serial.println("=== DEBUG: Verifying problematic strip addressing ===");
    
    // Test Pin 1 Strip 2 (strip_id = 2)
    Serial.printf("Pin 1 Strip 2 (strip_id=2): ");
    CRGBSet test_set_2 = getStripSet(2);
    Serial.printf("CRGBSet size=%d, ptr=%p\n", test_set_2.size(), &test_set_2[0]);
    Serial.printf("Strip config: pin=%d, offset=%d, length=%d, array_ptr=%p\n", 
                 strips[2].physical_pin, strips[2].start_offset, strips[2].length, strips[2].led_array_ptr);
    Serial.printf("Calculated start address: %p\n", strips[2].led_array_ptr + strips[2].start_offset);
    
    // Test Pin 4 Strip 2 (strip_id = 12) 
    Serial.printf("Pin 4 Strip 2 (strip_id=12): ");
    CRGBSet test_set_12 = getStripSet(12);
    Serial.printf("CRGBSet size=%d, ptr=%p\n", test_set_12.size(), &test_set_12[0]);
    Serial.printf("Strip config: pin=%d, offset=%d, length=%d, array_ptr=%p\n", 
                 strips[12].physical_pin, strips[12].start_offset, strips[12].length, strips[12].led_array_ptr);
    Serial.printf("Calculated start address: %p\n", strips[12].led_array_ptr + strips[12].start_offset);
    
    // Note: Array size verification removed due to extern declaration limitations
    
    Serial.println("Strip configuration initialized successfully");
    
    // Perform a simple LED addressing test
    testStripAddressing();
}

void testStripAddressing()
{
    Serial.println("=== TESTING STRIP ADDRESSING ===");
    
    // Test pin 1 strip 2 (strip_id = 2) - light up first 10 LEDs in red
    Serial.println("Testing Pin 1 Strip 2 (strip_id=2) - lighting first 10 LEDs red");
    CRGBSet test_strip_2 = getStripSet(2);
    Serial.printf("Strip 2 direction: %s, size reported: %d\n", 
                 strips[2].reverse_direction ? "REVERSE" : "FORWARD", test_strip_2.size());
    for (int i = 0; i < 10 && i < 122; i++) {  // Limit to known strip length
        test_strip_2[i] = CRGB::Red;
    }
    
    // Test pin 4 strip 2 (strip_id = 12) - light up first 10 LEDs in blue  
    Serial.println("Testing Pin 4 Strip 2 (strip_id=12) - lighting first 10 LEDs blue");
    CRGBSet test_strip_12 = getStripSet(12);
    Serial.printf("Strip 12 direction: %s, size reported: %d\n", 
                 strips[12].reverse_direction ? "REVERSE" : "FORWARD", test_strip_12.size());
    
    // For reverse strips, be extra careful with bounds
    int safe_limit = min(10, 122);  // Use known strip length instead of size()
    for (int i = 0; i < safe_limit; i++) {
        test_strip_12[i] = CRGB::Blue;
    }
    
    FastLED.show();
    Serial.println("Address test complete - check if LEDs are lighting correctly");
    
    delay(3000);  // Hold for 3 seconds
    
    // Clear the test LEDs using safe bounds
    for (int i = 0; i < 122; i++) {
        test_strip_2[i] = CRGB::Black;
        test_strip_12[i] = CRGB::Black;
    }
    FastLED.show();
    Serial.println("Test LEDs cleared");
}

CRGBSet getStripSet(uint8_t strip_id)
{
    // Create and return the appropriate CRGBSet based on direction configuration
    if (strip_id >= 22) {
        // Return first strip as fallback for invalid IDs
        strip_id = 0;
    }
    
    const StripConfig& strip = strips[strip_id];
    CRGB* strip_start = strip.led_array_ptr + strip.start_offset;
    
    // Always return forward direction CRGBSet - handle direction in access patterns
    // This avoids issues with CRGBSet size() calculation for reverse sets
    return CRGBSet(strip_start, strip.length);
}

// Helper function to get the actual strip length (not CRGBSet.size() which may be wrong for reverse sets)
uint16_t getStripLength(uint8_t strip_id)
{
    if (strip_id >= 22) {
        return 122; // Default strip length
    }
    return strips[strip_id].length;
}

// Helper function to access LEDs with direction handling
CRGB& getStripLED(uint8_t strip_id, uint16_t led_index)
{
    const StripConfig& strip = strips[strip_id];
    CRGB* strip_start = strip.led_array_ptr + strip.start_offset;
    
    // Apply direction logic
    uint16_t actual_index = strip.reverse_direction ? 
        (strip.length - 1 - led_index) : led_index;
    
    return strip_start[actual_index];
}

void updatePatternPalette(ChasePattern* pattern)
{
    // Convert CRGB palette to FastLED CRGBPalette16 for optimized operations
    if (pattern->palette_size > 0) {
        // Create a 16-color palette from the pattern's colors
        CRGB palette_colors[16];
        
        for (uint8_t i = 0; i < 16; i++) {
            // Repeat colors to fill the 16 slots, creating smooth gradients
            uint8_t source_index = (i * pattern->palette_size) / 16;
            palette_colors[i] = pattern->palette[source_index % pattern->palette_size];
        }
        
        // Create the FastLED palette
        pattern->fastled_palette = CRGBPalette16(
            palette_colors[0], palette_colors[1], palette_colors[2], palette_colors[3],
            palette_colors[4], palette_colors[5], palette_colors[6], palette_colors[7],
            palette_colors[8], palette_colors[9], palette_colors[10], palette_colors[11],
            palette_colors[12], palette_colors[13], palette_colors[14], palette_colors[15]
        );
    } else {
        // Default to a simple black palette if no colors provided
        pattern->fastled_palette = CRGBPalette16(CRGB::Black);
    }
}


unsigned long convertSpeedToDelay(uint8_t speed)
{
    // Clamp speed to valid range
    if (speed < 1)
        speed = 1;
    if (speed > 100)
        speed = 100;

    // Convert 1-100 scale to delay in milliseconds
    // For single chase to traverse 122 LEDs in 1 second at speed 100:
    // 1000ms / 122 positions = ~8ms per position
    // Speed 1 = 200ms delay (slowest, ~24 seconds to cross strip)
    // Speed 100 = 8ms delay (fastest, ~1 second to cross strip)

    // Target: 122ms total to cross 122-LED strip at speed 100
    // This means 1ms per LED position at max speed
    // Speed 1 = 20ms per LED (slowest, ~2.44 seconds to cross strip)
    // Speed 100 = 1ms per LED (fastest, ~122ms to cross strip)
    // Linear interpolation: delay = 20 - ((speed - 1) * (20 - 1) / 99)
    return 20 - ((speed - 1) * 19 / 99);
}

void addPatternToQueue(PatternType pattern_type, const PaletteConfig& palette_config,
    const StripGroupConfig& strip_config, uint8_t speed, unsigned long transition_delay, uint16_t transition_duration)
{
    if (pattern_queue.queue_size >= MAX_QUEUE_SIZE)
        return;

    ChasePattern& pattern = pattern_queue.patterns[pattern_queue.queue_size];

    pattern.pattern_type = pattern_type;

    // Copy palette from config
    for (uint8_t i = 0; i < palette_config.size && i < MAX_PALETTE_SIZE; i++) {
        pattern.palette[i] = palette_config.colors[i];
    }
    pattern.palette_size = palette_config.size;

    // Copy target strips from config
    for (uint8_t i = 0; i < strip_config.count && i < MAX_TARGET_STRIPS; i++) {
        pattern.target_strips[i] = strip_config.strips[i];
    }
    pattern.num_target_strips = strip_config.count;

    pattern.speed = speed;
    pattern.transition_delay = transition_delay * 1000; // Convert seconds to milliseconds
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

void runQueuedPattern()
{
    if (!pattern_queue.is_running || pattern_queue.queue_size == 0)
        return;

    updatePatternQueue();

    // Run all active or transitioning chase patterns FIRST (to set base pattern)
    bool any_pattern_updated = false;
    for (uint8_t i = 0; i < pattern_queue.queue_size; i++) {
        ChasePattern& pattern = pattern_queue.patterns[i];
        if (pattern.is_active || pattern.is_transitioning) {
            runPattern(&pattern);
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

void setupPatternProgram()
{
    // Clear any existing patterns
    clearPatternQueue();

    // Define palette configurations
    static PaletteConfig white_palette = { { CRGB::White }, 1 };

    static PaletteConfig warm_palette = { { CRGB::Red, CRGB::Orange, CRGB::Yellow }, 3 };

    static PaletteConfig cool_palette = { { CRGB::Blue, CRGB::Cyan, CRGB::Green }, 3 };

    static PaletteConfig rainbow_palette
        = { { CRGB::Red, CRGB::Orange, CRGB::Yellow, CRGB::Green, CRGB::Blue, CRGB::Purple }, 6 };

    static PaletteConfig sunset_palette = { { CRGB::Purple, CRGB::Magenta, CRGB::Orange, CRGB::Red }, 4 };

    // Define strip group configurations
    static StripGroupConfig all_strips
        = { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 }, 22 };

    static StripGroupConfig outside = { { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 }, 14 };

    static StripGroupConfig inside = { { 14, 15, 16, 17, 18, 19, 20, 21 }, 8 };

    static StripGroupConfig exterior_rings = { { 0, 1, 2, 11, 12, 13 }, 6 };

    // Add patterns to queue with different transition delays (in seconds) and speeds (1-100 scale)
    addPatternToQueue(PATTERN_CHASE, rainbow_palette, all_strips, 75,
        0); // Rainbow chase on all strips - medium-fast speed - starts immediately
    addPatternToQueue(PATTERN_SOLID, sunset_palette, exterior_rings, 1,
        10); // Solid sunset on exterior rings - speed irrelevant - starts after 10 seconds
    addPatternToQueue(PATTERN_SINGLE_CHASE, cool_palette, inside, 100,
        14); // White single chase on inside strips - MAX SPEED - starts after 14 seconds
    addPatternToQueue(PATTERN_SOLID, warm_palette, outside, 1,
        20); // Solid warm colors on outside strips - speed irrelevant - starts after 20 seconds
    addPatternToQueue(PATTERN_SINGLE_CHASE, rainbow_palette, exterior_rings, 100,
        30); // White single chase on exterior rings - MAX SPEED - starts after 30 seconds

    // Start the pattern program
    startPatternQueue();
}

void runPattern(ChasePattern* pattern)
{
    // Dispatch to appropriate pattern function based on type
    switch (pattern->pattern_type) {
    case PATTERN_SOLID:
        runSolidPattern(pattern);
        break;
    case PATTERN_SINGLE_CHASE:
        runSingleChasePattern(pattern);
        break;
    case PATTERN_RAINBOW:
        runRainbowPattern(pattern);
        break;
    case PATTERN_BREATHING:
        runBreathingPattern(pattern);
        break;
    case PATTERN_CHASE:
    default:
        runChasePatternLogic(pattern);
        break;
    }
}