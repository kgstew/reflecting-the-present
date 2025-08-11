#include "patterns.h"

PatternQueue pattern_queue = { .queue_size = 0, .current_pattern = 0, .pattern_start_time = 0, .is_running = false };

void addPatternToQueue(CRGB* palette, uint8_t palette_size, uint8_t* target_strips, uint8_t num_target_strips,
    uint16_t speed, unsigned long duration)
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
    pattern.duration = duration;

    pattern_queue.queue_size++;
}

void startPatternQueue()
{
    if (pattern_queue.queue_size > 0) {
        pattern_queue.current_pattern = 0;
        pattern_queue.pattern_start_time = current_time;
        pattern_queue.is_running = true;
    }
}

void stopPatternQueue() { pattern_queue.is_running = false; }

void clearPatternQueue()
{
    pattern_queue.queue_size = 0;
    pattern_queue.current_pattern = 0;
    pattern_queue.is_running = false;
}

void updatePatternQueue()
{
    if (!pattern_queue.is_running || pattern_queue.queue_size == 0)
        return;

    ChasePattern& current = pattern_queue.patterns[pattern_queue.current_pattern];
    unsigned long elapsed_time = current_time - pattern_queue.pattern_start_time;

    // Check if current pattern duration has elapsed
    if (elapsed_time >= current.duration) {
        // Move to next pattern
        pattern_queue.current_pattern++;

        // Check if we've reached the end of the queue
        if (pattern_queue.current_pattern >= pattern_queue.queue_size) {
            // Loop back to beginning or stop (you can modify this behavior)
            pattern_queue.current_pattern = 0;
        }

        // Reset start time for new pattern
        pattern_queue.pattern_start_time = current_time;
    }
}

void runQueuedChasePattern()
{
    if (!pattern_queue.is_running || pattern_queue.queue_size == 0)
        return;

    updatePatternQueue();

    ChasePattern& current = pattern_queue.patterns[pattern_queue.current_pattern];
    chasePattern(
        current.target_strips, current.num_target_strips, current.palette, current.palette_size, current.speed);
}

void chasePattern(
    uint8_t* target_strips, uint8_t num_target_strips, CRGB* palette, uint8_t palette_size, uint16_t speed)
{
    static unsigned long last_update = 0;
    static uint16_t chase_position = 0;

    if (current_time - last_update >= speed) {
        last_update = current_time;

        // Calculate total length of all target strips
        uint16_t total_pattern_length = 0;
        for (uint8_t i = 0; i < num_target_strips; i++) {
            uint8_t strip_id = target_strips[i];
            if (strip_id < 22) {
                total_pattern_length += strip_lengths[strip_id];
            }
        }

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
    addPatternToQueue(warm_palette, 3, all_strips, 22, 50, 10000); // 10 seconds - warm colors on all strips
    addPatternToQueue(cool_palette, 3, outside, 14, 30, 8000); // 8 seconds - cool colors on first half, faster
    addPatternToQueue(rainbow_palette, 6, inside, 8, 40, 12000); // 12 seconds - rainbow on second half
    addPatternToQueue(sunset_palette, 4, exterior_rings, 6, 80, 6000); // 6 seconds - sunset colors on corners, slower
    addPatternToQueue(rainbow_palette, 6, all_strips, 22, 25, 15000); // 15 seconds - fast rainbow on all strips

    // Start the pattern program
    startPatternQueue();
}