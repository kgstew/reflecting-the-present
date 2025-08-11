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

struct ChasePattern {
    CRGB palette[MAX_PALETTE_SIZE];
    uint8_t palette_size;
    uint8_t target_strips[MAX_TARGET_STRIPS];
    uint8_t num_target_strips;
    uint16_t speed;
    unsigned long duration;
};

struct PatternQueue {
    ChasePattern patterns[MAX_QUEUE_SIZE];
    uint8_t queue_size;
    uint8_t current_pattern;
    unsigned long pattern_start_time;
    bool is_running;
};

// External references to global variables from main.cpp
extern unsigned long current_time;
extern uint16_t strip_lengths[];
extern uint8_t strip_map[];
extern PinConfig pin_configs[];

// Pattern queue functions
void addPatternToQueue(CRGB* palette, uint8_t palette_size, uint8_t* target_strips, uint8_t num_target_strips, uint16_t speed, unsigned long duration);
void startPatternQueue();
void stopPatternQueue();
void clearPatternQueue();
void updatePatternQueue();
void runQueuedChasePattern();

// Chase pattern function
void chasePattern(uint8_t* target_strips, uint8_t num_target_strips, CRGB* palette, uint8_t palette_size, uint16_t speed);

// Example program setup
void setupPatternProgram();

#endif