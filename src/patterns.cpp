#include "patterns.h"

PatternQueue pattern_queue = { .queue_size = 0, .queue_start_time = 0, .is_running = false };

CRGB& getLED(uint8_t strip_id, uint16_t led_index)
{
    // Direct LED access using new unified configuration
    const StripConfig& strip = strips[strip_id];

    // Apply direction logic
    uint16_t actual_index = strip.reverse_direction ? (strip.length - 1 - led_index) : led_index;

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

        Serial.printf("Strip %d: %s\n", i, strips[i].reverse_direction ? "REVERSED" : "FORWARD");
    }
}

void testStripAddressing()
{
    Serial.println("=== TESTING STRIP ADDRESSING ===");

    // Test pin 1 strip 2 (strip_id = 2) - light up first 10 LEDs in red
    Serial.println("Testing Pin 1 Strip 2 (strip_id=2) - lighting first 10 LEDs red");
    CRGBSet test_strip_2 = getStripSet(2);
    Serial.printf("Strip 2 direction: %s, size reported: %d\n", strips[2].reverse_direction ? "REVERSE" : "FORWARD",
        test_strip_2.size());
    for (int i = 0; i < 10 && i < 122; i++) { // Limit to known strip length
        test_strip_2[i] = CRGB::Red;
    }

    // Test pin 4 strip 2 (strip_id = 12) - light up first 10 LEDs in blue
    Serial.println("Testing Pin 4 Strip 2 (strip_id=12) - lighting first 10 LEDs blue");
    CRGBSet test_strip_12 = getStripSet(12);
    Serial.printf("Strip 12 direction: %s, size reported: %d\n", strips[12].reverse_direction ? "REVERSE" : "FORWARD",
        test_strip_12.size());

    // For reverse strips, be extra careful with bounds
    int safe_limit = min(10, 122); // Use known strip length instead of size()
    for (int i = 0; i < safe_limit; i++) {
        test_strip_12[i] = CRGB::Blue;
    }

    FastLED.show();
    Serial.println("Address test complete - check if LEDs are lighting correctly");

    delay(3000); // Hold for 3 seconds

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

// Helper function to access LEDs with direction handling
CRGB& getStripLED(uint8_t strip_id, uint16_t led_index)
{
    const StripConfig& strip = strips[strip_id];
    CRGB* strip_start = strip.led_array_ptr + strip.start_offset;

    // Apply direction logic
    uint16_t actual_index = strip.reverse_direction ? (strip.length - 1 - led_index) : led_index;

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
        pattern->fastled_palette = CRGBPalette16(palette_colors[0], palette_colors[1], palette_colors[2],
            palette_colors[3], palette_colors[4], palette_colors[5], palette_colors[6], palette_colors[7],
            palette_colors[8], palette_colors[9], palette_colors[10], palette_colors[11], palette_colors[12],
            palette_colors[13], palette_colors[14], palette_colors[15]);
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
    
    // Initialize parameters to zero (caller should use the overload with PatternParams for custom config)
    memset(&pattern.params, 0, sizeof(PatternParams));

    pattern_queue.queue_size++;
}


void addPatternToQueue(PatternType pattern_type, const PaletteConfig& palette_config,
    const StripGroupConfig& strip_config, uint8_t speed, unsigned long transition_delay,
    uint16_t transition_duration, const PatternParams& params)
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
    
    // Copy custom parameters
    pattern.params = params;

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

    // Add patterns to queue with custom parameter configurations
    const uint8_t start_time = 0;
    
    // Pattern 1: Enhanced chase with custom parameters
    PatternParams chaseParams = {};
    chaseParams.chase.chase_width = 8;
    chaseParams.chase.fade_rate = 0.7f;
    chaseParams.chase.bounce_mode = false;
    chaseParams.chase.color_shift = true;
    addPatternToQueue(PATTERN_CHASE, rainbow_palette, all_strips, 15, start_time, 1000, chaseParams);

    // Pattern 2: Custom breathing with fine-tuned parameters
    const uint8_t pattern_two_delay = start_time + 10;
    PatternParams breathingParams = {};
    breathingParams.breathing.min_brightness = 0.15f;  // 15% minimum brightness
    breathingParams.breathing.max_brightness = 0.95f;  // 95% maximum brightness
    breathingParams.breathing.color_cycle_speed = 0.1f; // Very slow color cycling
    addPatternToQueue(PATTERN_BREATHING, cool_palette, inside, 5, pattern_two_delay, 1000, breathingParams);

    // Pattern 3: Enhanced pinwheel with custom rotation
    const uint8_t pattern_three_delay = pattern_two_delay + 10;
    PatternParams pinwheelParams = {};
    pinwheelParams.pinwheel.rotation_speed = 2.5f;      // 2.5x faster rotation
    pinwheelParams.pinwheel.color_cycles = 5.0f;        // 5 color cycles
    pinwheelParams.pinwheel.radial_fade = true;         // Enable radial fade
    pinwheelParams.pinwheel.center_brightness = 0.8f;   // 80% center brightness
    addPatternToQueue(PATTERN_PINWHEEL, sunset_palette, exterior_rings, 80, pattern_three_delay, 1000, pinwheelParams);

    // Pattern 4: Fast rainbow with custom cycle speed
    const uint8_t pattern_four_delay = pattern_three_delay + 10;
    PatternParams rainbowParams = {};
    rainbowParams.rainbow.cycle_speed = 2.0f;           // 2x faster cycling
    rainbowParams.rainbow.vertical_mode = true;         // Vertical rainbow mode
    addPatternToQueue(PATTERN_RAINBOW, rainbow_palette, all_strips, 50, pattern_four_delay, 1000, rainbowParams);

    // addPatternToQueue(PATTERN_SOLID, sunset_palette, exterior_rings, 1,
    //     60); // Solid sunset on exterior rings - speed irrelevant - starts after 10 seconds
    // addPatternToQueue(PATTERN_SINGLE_CHASE, cool_palette, inside, 100,
    //     30); // White single chase on inside strips - MAX SPEED - starts after 14 seconds
    // addPatternToQueue(PATTERN_SOLID, warm_palette, outside, 1,
    //     40); // Solid warm colors on outside strips - speed irrelevant - starts after 20 seconds
    // addPatternToQueue(PATTERN_SINGLE_CHASE, rainbow_palette, exterior_rings, 100,
    //     50); // White single chase on exterior rings - MAX SPEED - starts after 30 seconds

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
    case PATTERN_RAINBOW_HORIZONTAL:
        runRainbowHorizontalPattern(pattern);
        break;
    case PATTERN_BREATHING:
        runBreathingPattern(pattern);
        break;
    case PATTERN_PINWHEEL:
        runPinwheelPattern(pattern);
        break;
    case PATTERN_CHASE:
    default:
        runChasePattern(pattern);
        break;
    }
}