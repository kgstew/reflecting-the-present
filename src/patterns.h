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

struct StripConfig {
    uint8_t physical_pin;        // ESP32 pin number (13, 5, 19, etc.)
    uint8_t pin_index;           // Which pin array (0-5)
    uint16_t start_offset;       // LED offset within pin array
    uint16_t length;             // Number of LEDs in this strip
    bool reverse_direction;      // true = forward, false = reverse
    CRGB* led_array_ptr;         // Direct pointer to pin's LED array
    // CRGBSets will be created on-demand in getStripSet() function
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

enum FlashBulbState { FLASHBULB_INACTIVE, FLASHBULB_FLASH, FLASHBULB_FADE_TO_BLACK, FLASHBULB_TRANSITION_BACK };

enum PatternType { PATTERN_CHASE, PATTERN_SOLID, PATTERN_SINGLE_CHASE, PATTERN_RAINBOW, PATTERN_BREATHING };

struct ChasePattern {
    PatternType pattern_type;
    CRGB palette[MAX_PALETTE_SIZE];
    uint8_t palette_size;
    CRGBPalette16 fastled_palette;    // FastLED palette for optimized operations
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
extern PinConfig pin_configs[];
extern StripConfig strips[];
extern CRGB pin1_leds[];
extern CRGB pin2_leds[];
extern CRGB pin3_leds[];
extern CRGB pin4_leds[];
extern CRGB pin5_leds[];
extern CRGB pin6_leds[];

// External references to pattern managers
extern PatternQueue pattern_queue;
extern FlashBulbManager flashbulb_manager;


// New strip configuration functions
void initializeStripConfigs();
void configureStripDirections();
CRGB& getLED(uint8_t strip_id, uint16_t led_index);
CRGBSet getStripSet(uint8_t strip_id);
uint16_t getStripLength(uint8_t strip_id);
CRGB& getStripLED(uint8_t strip_id, uint16_t led_index);

// FastLED optimization functions
void updatePatternPalette(ChasePattern* pattern);

// Universal speed conversion (1=slowest, 100=fastest)
unsigned long convertSpeedToDelay(uint8_t speed);

// Pattern queue functions
void addPatternToQueue(PatternType pattern_type, const PaletteConfig& palette_config,
    const StripGroupConfig& strip_config, uint8_t speed, unsigned long transition_delay,
    uint16_t transition_duration = 1000);
void startPatternQueue();
void stopPatternQueue();
void clearPatternQueue();
void updatePatternQueue();
void runQueuedPattern();

// Main pattern handler
void runPattern(ChasePattern* pattern);

// Chase pattern functions
void chasePattern(
    uint8_t* target_strips, uint8_t num_target_strips, CRGB* palette, uint8_t palette_size, uint8_t speed);
void runChasePatternLogic(ChasePattern* pattern);
bool isStripActiveInFlashBulb(uint8_t strip_id);

// Solid pattern functions
void runSolidPattern(ChasePattern* pattern);

// Single chase pattern functions
void runSingleChasePattern(ChasePattern* pattern);

// FastLED optimized pattern functions
void runRainbowPattern(ChasePattern* pattern);
void runBreathingPattern(ChasePattern* pattern);

// FlashBulb pattern functions
void initFlashBulbManager();
void addFlashBulbPattern(uint8_t* target_strips, uint8_t num_target_strips);
void triggerFlashBulb(uint8_t pattern_index);
void updateFlashBulbPatterns();
void runFlashBulbPattern(FlashBulbPattern* pattern);

// Example program setup
void setupPatternProgram();

#endif