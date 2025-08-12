#include "patterns.h"

FlashBulbManager flashbulb_manager = { .pattern_count = 0 };

void initFlashBulbManager()
{
    flashbulb_manager.pattern_count = 0;
    for (uint8_t i = 0; i < MAX_FLASHBULB_PATTERNS; i++) {
        flashbulb_manager.patterns[i].state = FLASHBULB_INACTIVE;
        flashbulb_manager.patterns[i].num_target_strips = 0;
        flashbulb_manager.patterns[i].saved_color_count = 0;
    }
}

void addFlashBulbPattern(uint8_t* target_strips, uint8_t num_target_strips)
{
    if (flashbulb_manager.pattern_count >= MAX_FLASHBULB_PATTERNS)
        return;

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

void triggerFlashBulb(uint8_t pattern_index)
{
    if (pattern_index >= flashbulb_manager.pattern_count)
        return;

    FlashBulbPattern& pattern = flashbulb_manager.patterns[pattern_index];

    // Save current colors for transition back
    pattern.saved_color_count = 0;
    for (uint8_t i = 0; i < pattern.num_target_strips; i++) {
        uint8_t strip_id = pattern.target_strips[i];
        if (strip_id >= 22)
            continue;

        // Save current colors
        for (uint16_t led = 0; led < strips[strip_id].length && pattern.saved_color_count < (MAX_TARGET_STRIPS * 122); led++) {
            pattern.saved_colors[pattern.saved_color_count] = getLED(strip_id, led);
            pattern.saved_color_count++;
        }
    }

    // Set all target LEDs to white immediately
    for (uint8_t i = 0; i < pattern.num_target_strips; i++) {
        uint8_t strip_id = pattern.target_strips[i];
        if (strip_id >= 22)
            continue;

        // Set to full white
        for (uint16_t led = 0; led < strips[strip_id].length; led++) {
            getLED(strip_id, led) = CRGB::White;
        }
    }

    // Start the flash sequence
    pattern.state = FLASHBULB_FLASH;
    pattern.start_time = current_time;

    FastLED.show();
}

void updateFlashBulbPatterns()
{
    for (uint8_t i = 0; i < flashbulb_manager.pattern_count; i++) {
        FlashBulbPattern& pattern = flashbulb_manager.patterns[i];

        if (pattern.state != FLASHBULB_INACTIVE) {
            runFlashBulbPattern(&pattern);
        }
    }
}

void runFlashBulbPattern(FlashBulbPattern* pattern)
{
    unsigned long elapsed = current_time - pattern->start_time;

    switch (pattern->state) {
    case FLASHBULB_FLASH:
        if (elapsed < 100) { // Hold white flash for 100ms
            // Keep LEDs white during flash period
            for (uint8_t i = 0; i < pattern->num_target_strips; i++) {
                uint8_t strip_id = pattern->target_strips[i];
                if (strip_id >= 22)
                    continue;

                // Ensure LEDs stay white during flash
                for (uint16_t led = 0; led < strips[strip_id].length; led++) {
                    getLED(strip_id, led) = CRGB::White;
                }
            }
        } else {
            // Flash period complete, start fade to black
            pattern->state = FLASHBULB_FADE_TO_BLACK;
            pattern->start_time = current_time;
            Serial.println("FlashBulb: Starting fade to black phase");
        }
        break;

    case FLASHBULB_FADE_TO_BLACK:
        if (elapsed < 5000) { // 5 second fade to black
            uint8_t fade_amount = (elapsed * 255) / 5000;

            // Apply fade to all target strips
            for (uint8_t i = 0; i < pattern->num_target_strips; i++) {
                uint8_t strip_id = pattern->target_strips[i];
                if (strip_id >= 22)
                    continue;

                // Fade from white to black
                for (uint16_t led = 0; led < strips[strip_id].length; led++) {
                    CRGB white = CRGB::White;
                    CRGB black = CRGB::Black;
                    getLED(strip_id, led) = white.lerp8(black, fade_amount);
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
                if (strip_id >= 22)
                    continue;

                // Ensure black at transition start
                for (uint16_t led = 0; led < strips[strip_id].length; led++) {
                    getLED(strip_id, led) = CRGB::Black;
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
                if (strip_id >= 22)
                    continue;

                // Blend from black to the current chase pattern colors
                for (uint16_t led = 0; led < strips[strip_id].length; led++) {
                    // Get the current color that the chase pattern set
                    CRGB current_chase_color = getLED(strip_id, led);
                    CRGB black = CRGB::Black;

                    // Blend from black to the current chase pattern color based on transition progress
                    getLED(strip_id, led) = black.lerp8(current_chase_color, transition_amount);
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