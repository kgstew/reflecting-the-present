# Reflecting the Present - LED Art Installation

## Project Overview

This is an ESP32-based LED art installation that controls 22 LED strips arranged across 6 GPIO pins. The project uses WS2812B addressable LEDs (NeoPixels) and implements a sophisticated pattern management system for creating dynamic lighting effects.

## Hardware Configuration

### Microcontroller
- **Platform**: ESP32 DevKit V1
- **Framework**: Arduino
- **Main Library**: FastLED v3.10.1

### LED Strip Layout
- **Total LEDs**: 2,684 LEDs across 22 strips
- **Strip Length**: 122 LEDs per strip
- **LED Type**: WS2812B (NeoPixels)
- **Color Order**: GRB
- **Brightness**: 255 (max)

### Pin Configuration
- **PIN1 (GPIO 5)**: 3 strips (366 total LEDs)
- **PIN2 (GPIO 18)**: 4 strips (488 total LEDs) 
- **PIN3 (GPIO 19)**: 4 strips (488 total LEDs)
- **PIN4 (GPIO 21)**: 3 strips (366 total LEDs)
- **PIN5 (GPIO 22)**: 4 strips (488 total LEDs)
- **PIN6 (GPIO 23)**: 4 strips (488 total LEDs)

## Code Architecture

### Core Files
- `src/main.cpp:33-71`: Hardware setup and FastLED initialization
- `src/main.cpp:73-135`: Main loop with demo patterns
- `include/patterns.h`: Pattern system definitions and interfaces
- `src/patterns.cpp`: Pattern implementation and strip management

### Pattern System

#### StripPatternManager Class
The core pattern management system (`src/patterns.cpp:25-48`) handles:
- Individual strip control with timing
- Pattern transitions and duration management
- Reverse direction support for strips
- Global rainbow synchronization

#### Available Patterns
1. **PATTERN_RAINBOW_CHASE** (`src/patterns.cpp:99-113`): Default animated rainbow effect
2. **PATTERN_SOLID_COLOR** (`src/patterns.cpp:115-119`): Static color fill
3. **PATTERN_OFF** (`src/patterns.cpp:121-125`): All LEDs off
4. **PATTERN_FLASHBULB** (`src/patterns.cpp:127-164`): White flash that fades to rainbow

#### Pattern State Management
Each strip maintains state (`include/patterns.h:16-23`):
- Pattern type and timing
- Color information
- Active/inactive status
- Reverse direction flag

## Key Features

### Dynamic Pattern Control
- Individual strip addressing (22 strips total)
- Timed pattern duration with automatic reversion
- Delayed pattern activation
- Real-time pattern switching

### Demo System
The main loop (`src/main.cpp:79-128`) includes a demonstration system that cycles through different pattern combinations every 3 seconds, showcasing the installation's capabilities.

### Hardware Simulation
The project includes Wokwi simulation support:
- `diagram.json`: Complete hardware layout for simulation
- `wokwi.toml`: Simulation configuration
- Accessible at: https://wokwi.com/projects/287950823116243464

## Development Environment

### PlatformIO Configuration
- Target: ESP32 development board
- Build system: PlatformIO
- Dependencies managed via `platformio.ini`

### Build Commands
```bash
# Build the project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

## Recent Development

Based on git history, recent work has focused on:
- Creating a reverse configuration system for directional control
- Adding flashbulb pattern implementation
- Developing the pattern state management system
- Removing legacy pattern management code

## Installation Notes

This appears to be an art installation called "Reflecting the Present" with a sophisticated LED control system designed for creating immersive lighting experiences. The modular pattern system allows for complex choreographed effects across the 22-strip array.

---

# LED Controller Architecture Improvements

## Current Issues Analysis

### 1. Hardcoded Demo Logic (`src/main.cpp:89-122`)
The transition timing and patterns are embedded in the main loop, making it inflexible and difficult to modify.

### 2. No External Message Handling
No system exists to receive flashbulb triggers from another micro controller without disrupting the ongoing transition patterns.

### 3. Abrupt Pattern Switching
Patterns switch instantly without smooth transitions or blend periods between different color palettes and speeds.

### 4. Limited Pattern Configuration
Patterns have minimal customization options (only basic speed, color, duration parameters).

## Recommended Architecture Changes

### 1. Message Handler System
```cpp
class MessageHandler {
private:
    volatile bool flashbulb_requested = false;
    uint8_t* pending_strips = nullptr;
    uint8_t pending_count = 0;
    
public:
    void checkExternalMessages(); // Check serial/I2C for commands
    bool hasFlashbulbRequest();
    void triggerFlashbulb(uint8_t* strips, uint8_t count);
    void clearFlashbulbRequest();
};
```

### 2. Pattern Transition Queue
```cpp
struct PatternTransition {
    uint8_t* strip_ids;
    uint8_t strip_count;
    PatternType target_pattern;
    ColorPalette palette;
    uint16_t fade_duration_ms;
    uint32_t start_delay_ms;
    uint32_t duration;
};

class TransitionManager {
private:
    PatternTransition* transition_queue;
    uint8_t queue_size, queue_head, queue_tail;
    
public:
    void queueTransition(const PatternTransition& transition);
    void processQueue(uint32_t current_time);
    bool isTransitioning(uint8_t strip_id);
};
```

### 3. Enhanced Pattern Parameters
```cpp
struct EnhancedStripState : StripState {
    float fade_progress;           // 0.0-1.0 for smooth transitions
    PatternType transition_target; // Target pattern during fade
    ColorPalette transition_palette;
    uint32_t transition_start_time;
    uint32_t transition_duration;
    bool is_transitioning;
};
```

### 4. Interrupt-Safe Flashbulb System
Move flashbulb triggers to interrupt handler that sets flags, processed in main loop:

```cpp
// In interrupt handler
void IRAM_ATTR onExternalTrigger() {
    MessageHandler::triggerFlashbulbInterrupt();
}

// In main loop
if (messageHandler.hasFlashbulbRequest()) {
    uint8_t strips[] = {0, 3, 7, 11, 14, 18};
    StripPatternManager::queueFlashbulb(strips, 6);
    messageHandler.clearFlashbulbRequest();
}
```

### 5. Separated Demo Controller
```cpp
class DemoController {
private:
    uint32_t last_transition_time = 0;
    uint8_t current_demo_step = 0;
    
public:
    void update(uint32_t current_time, TransitionManager& transition_mgr);
    void setDemoSequence(const PatternTransition* sequence, uint8_t count);
};
```

## Key Benefits

- **Non-blocking external triggers**: Flashbulb can interrupt without breaking transitions
- **Smooth pattern blending**: Gradual fades between color palettes and speeds  
- **Flexible scheduling**: Queue multiple pattern changes with precise timing
- **Clean separation**: Demo logic separated from core pattern management
- **Extensible**: Easy to add new patterns and transition types

## Refactored Main Loop
The refactored `main.cpp` would be much cleaner:
```cpp
void loop() {
    uint32_t current_time = millis();
    
    messageHandler.checkExternalMessages();
    if (messageHandler.hasFlashbulbRequest()) {
        // Handle flashbulb trigger
    }
    
    demoController.update(current_time, transitionManager);
    transitionManager.processQueue(current_time);
    StripPatternManager::updateAllStrips(pin_configs, strip_lengths, NUM_PINS, current_time);
    
    FastLED.show();
}
```

## Implementation Priority
1. Pattern Transition Queue/Scheduler System
2. Enhanced Pattern Parameters  
3. Message Handler System
4. Interrupt-Safe Flashbulb System
5. Demo Controller Separation
6. Smooth Transitions Between Patterns