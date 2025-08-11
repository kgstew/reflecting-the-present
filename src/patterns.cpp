#include "patterns.h"

PatternQueue pattern_queue = { .queue_size = 0, .queue_start_time = 0, .is_running = false };

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
    case PATTERN_CHASE:
    default:
        runChasePatternLogic(pattern);
        break;
    }
}