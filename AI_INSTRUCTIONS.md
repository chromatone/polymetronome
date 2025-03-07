# AI Development Instructions

## Module Structure

Break down the project into these independent modules:

1. **Hardware Drivers** (`/src/drivers/`)

   - `Solenoid.h/cpp` - Solenoid control with timing
   - `Display.h/cpp` - OLED display operations
   - `Encoder.h/cpp` - Rotary encoder handling
   - `PiezoSensor.h/cpp` - Tap tempo input

2. **Core Logic** (`/src/core/`)

   - `Timer.h/cpp` - Precise timing management
   - `Pattern.h/cpp` - Beat pattern storage and manipulation
   - `Channel.h/cpp` - Single metronome channel logic
   - `Euclidean.h/cpp` - Euclidean rhythm generator

3. **UI Components** (`/src/ui/`)

   - `Layout.h/cpp` - Screen layout management
   - `Menu.h/cpp` - Navigation system
   - `Widgets.h/cpp` - Reusable UI elements
   - `Indicators.h/cpp` - Progress bars and indicators

4. **State Management** (`/src/state/`)
   - `GlobalState.h/cpp` - Application state
   - `Settings.h/cpp` - Persistent settings
   - `Events.h/cpp` - Event bus system

## Development Workflow

1. Start with hardware drivers

   ```cpp
   // Example driver interface
   class Solenoid {
       public:
           void trigger(uint8_t duration_ms);
           void update();  // Called in main loop
           bool isActive() const;
   };
   ```

2. Build core logic modules

   ```cpp
   // Example pattern interface
   class Pattern {
       public:
           void setLength(uint8_t steps);
           void toggleStep(uint8_t step);
           BeatType getStep(uint8_t step) const;
           void generateEuclidean(uint8_t steps, uint8_t fills);
   };
   ```

3. Create UI components

   ```cpp
   // Example widget interface
   class ProgressBar {
       public:
           void setProgress(float value);  // 0.0-1.0
           void setPosition(uint8_t x, uint8_t y);
           void draw(Display* display);
   };
   ```

4. Implement state management
   ```cpp
   // Example event system
   class EventBus {
       public:
           void emit(EventType type, void* data);
           void subscribe(EventType type, Callback cb);
   };
   ```

## Testing Strategy

1. Create individual test files for each module
2. Test hardware interfaces with mock objects
3. Validate timing with oscilloscope
4. Test UI with display simulator

## AI Development Tips

1. Keep files under 200 lines
2. One class = one responsibility
3. Use clear naming conventions
4. Document all public interfaces
5. Include usage examples
6. Separate configuration into config.h

## Development Order

1. Hardware Layer

   ```
   1. Solenoid control
   2. Display basics
   3. Encoder input
   4. Piezo sensing
   ```

2. Core Features

   ```
   1. Basic timing
   2. Single channel
   3. Pattern storage
   4. Multi-channel sync
   ```

3. User Interface

   ```
   1. Layout system
   2. Basic navigation
   3. Pattern editor
   4. Visual feedback
   ```

4. Advanced Features
   ```
   1. Tap tempo
   2. Euclidean generator
   3. Settings storage
   4. MIDI sync
   ```

## Configuration Template

```cpp
// config.h
#pragma once

// Hardware
#define DISPLAY_SDA 21
#define DISPLAY_SCL 22

// Timing
#define MIN_BPM 20
#define MAX_BPM 500

// UI
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Features
#define MAX_CHANNELS 3
#define MAX_STEPS 16
```

## Example Module Request

When requesting AI assistance for a module, use this format:

```
Please help implement the [Module] with these requirements:
1. Interface: [Key methods and properties]
2. Dependencies: [Required modules]
3. Configuration: [Related constants]
4. Example usage: [Code snippet]
```

## Version Control Strategy

1. One feature = one branch
2. Small, focused commits
3. Test before merge
4. Tag stable versions

## Documentation Standards

1. Every header file starts with:

```cpp
/**
 * @file ModuleName.h
 * @brief Short description
 * @details Detailed explanation
 */
```

2. Every class/method needs:

```cpp
/**
 * @brief What it does
 * @param name Description
 * @return What it returns
 */
```

## Memory Management

1. Use stack allocation when possible
2. Avoid dynamic allocation during playback
3. Pre-allocate pattern storage
4. Monitor stack usage

## Error Handling

1. Use return codes for hardware
2. Validate all inputs
3. Log errors to Serial
4. Fail gracefully

## Performance Tips

1. Use FreeRTOS tasks for timing
2. Minimize display updates
3. Use interrupt for encoder
4. Batch UI updates
