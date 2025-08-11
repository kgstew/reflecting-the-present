#ifndef PATTERNS_H
#define PATTERNS_H

#include <FastLED.h>

struct PinConfig {
    uint8_t pin;
    uint8_t num_strips;
    uint16_t leds_per_strip;
    uint16_t total_leds;
    CRGB* led_array;
};

#define MAX_QUEUE_SIZE 10
#define MAX_PALETTE_SIZE 16
#define MAX_TARGET_STRIPS 22
#define MAX_FLASHBULB_PATTERNS 5

struct PaletteConfig {
    CRGB colors[MAX_PALETTE_SIZE];
    uint8_t size;
};

struct StripGroupConfig {
    uint8_t strips[MAX_TARGET_STRIPS];
    uint8_t count;
};

enum FlashBulbState {
    FLASHBULB_INACTIVE,
    FLASHBULB_FLASH,
    FLASHBULB_FADE_TO_BLACK,
    FLASHBULB_TRANSITION_BACK
};

enum PatternType {
    PATTERN_CHASE,
    PATTERN_SOLID,
    PATTERN_SINGLE_CHASE
};

struct ChasePattern {
    PatternType pattern_type;
    CRGB palette[MAX_PALETTE_SIZE];
    uint8_t palette_size;
    uint8_t target_strips[MAX_TARGET_STRIPS];
    uint8_t num_target_strips;
    uint8_t speed; // 1-100 scale (1=slowest, 100=fastest)
    unsigned long transition_delay;
    unsigned long last_update;
    uint16_t chase_position;
    bool is_active;
    bool is_transitioning;
    unsigned long transition_start_time;
    uint16_t transition_duration;
};

struct FlashBulbPattern {
    uint8_t target_strips[MAX_TARGET_STRIPS];
    uint8_t num_target_strips;
    FlashBulbState state;
    unsigned long start_time;
    CRGB saved_colors[MAX_TARGET_STRIPS * 122]; // Store original colors for transition back
    uint16_t saved_color_count;
};

struct PatternQueue {
    ChasePattern patterns[MAX_QUEUE_SIZE];
    uint8_t queue_size;
    unsigned long queue_start_time;
    bool is_running;
};

struct FlashBulbManager {
    FlashBulbPattern patterns[MAX_FLASHBULB_PATTERNS];
    uint8_t pattern_count;
};

// External references to global variables from main.cpp
extern unsigned long current_time;
extern uint16_t strip_lengths[];
extern uint8_t strip_map[];
extern PinConfig pin_configs[];

// External references to pattern managers
extern PatternQueue pattern_queue;
extern FlashBulbManager flashbulb_manager;

// Universal speed conversion (1=slowest, 100=fastest)
unsigned long convertSpeedToDelay(uint8_t speed);

// Pattern queue functions
void addPatternToQueue(PatternType pattern_type, const PaletteConfig& palette_config, const StripGroupConfig& strip_config, uint8_t speed, unsigned long transition_delay, uint16_t transition_duration = 1000);
void startPatternQueue();
void stopPatternQueue();
void clearPatternQueue();
void updatePatternQueue();
void runQueuedChasePattern();

// Main pattern handler
void runPattern(ChasePattern* pattern);

// Chase pattern functions
void chasePattern(uint8_t* target_strips, uint8_t num_target_strips, CRGB* palette, uint8_t palette_size, uint8_t speed);
void runChasePatternLogic(ChasePattern* pattern);
bool isStripActiveInFlashBulb(uint8_t strip_id);

// Solid pattern functions
void runSolidPattern(ChasePattern* pattern);

// Single chase pattern functions
void runSingleChasePattern(ChasePattern* pattern);

// FlashBulb pattern functions
void initFlashBulbManager();
void addFlashBulbPattern(uint8_t* target_strips, uint8_t num_target_strips);
void triggerFlashBulb(uint8_t pattern_index);
void updateFlashBulbPatterns();
void runFlashBulbPattern(FlashBulbPattern* pattern);

// Example program setup
void setupPatternProgram();

#endif