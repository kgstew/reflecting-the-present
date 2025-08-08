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