# LED Strip Configuration Simplification

## Migration Plan: Step-by-Step Implementation

### Step 1: Create New StripConfig Structure
- [ ] Add new `StripConfig` struct to `patterns.h`
- [ ] Define unified configuration array in `main.cpp`
- [ ] Populate array with current strip data (pin mappings, lengths, directions, offsets)

### Step 2: Add Helper Functions
- [ ] Implement `getLED(strip_id, led_index)` function for direct LED access
- [ ] Add `initializeStripConfigs()` function to auto-calculate offsets and pin relationships
- [ ] Create validation function to verify configuration consistency

### Step 3: Update Pattern Functions (Phase 1)
- [ ] Update `chase_patterns.cpp` to use new `getLED()` function
- [ ] Update `single_chase_patterns.cpp` to use new configuration
- [ ] Update `solid_patterns.cpp` to use new structure

### Step 4: Update Pattern Functions (Phase 2)  
- [ ] Update `flashbulb_patterns.cpp` to use new configuration
- [ ] Update any remaining pattern-related functions in `patterns.cpp`
- [ ] Test all pattern types work correctly with new system

### Step 5: Remove Legacy Arrays
- [ ] Remove `strip_map[]` array from `main.cpp`
- [ ] Remove `strip_offsets[]` array from `main.cpp`
- [ ] Remove `strip_directions[]` array from `main.cpp` 
- [ ] Remove `strip_lengths[]` array from `main.cpp`
- [ ] Remove `calculateStripOffsets()` function from `patterns.cpp`

### Step 6: Update Headers and Cleanup
- [ ] Remove extern declarations for old arrays from `patterns.h`
- [ ] Remove old function declarations from `patterns.h`
- [ ] Update `setup()` to call new initialization function
- [ ] Remove call to `calculateStripOffsets()` from `setup()`

### Step 7: Testing and Validation
- [ ] Compile and verify no build errors
- [ ] Test basic pattern functionality
- [ ] Test FlashBulb patterns work correctly
- [ ] Test sensor trigger functionality
- [ ] Verify WebSocket communication still works

### Step 8: Optional Optimizations
- [ ] Consider adding PinManager structure for better pin-level control
- [ ] Add compile-time validation of strip configurations
- [ ] Consider memory pool allocation for better performance

## Benefits Expected
- **Memory Reduction**: ~88+ bytes saved from eliminated arrays
- **Code Clarity**: Single source of truth for strip configuration
- **Performance**: Direct LED access without runtime calculations
- **Maintainability**: Easier to modify strip configurations
- **Type Safety**: Reduced chance of array index errors

## Current Configuration to Preserve
```
Pins: 13, 5, 19, 23, 18, 12
Strip counts per pin: 3, 4, 4, 3, 4, 4 (total: 22 strips)
Strip length: 122 LEDs each
Strip directions: Currently all forward (true)
```