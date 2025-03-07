# Polymetronome Project Specification

## Core Concept

A modular multi-track hardware metronome system capable of running multiple independent rhythm patterns synchronized to a master tempo. Built on ESP32 with OLED display and precise solenoid actuation.

## Hardware Specifications

### Required Components

- ESP32 Development Board
- SH1106 128x64 OLED Display (I2C)
- Rotary Encoder with push button
- 2 Control buttons (Start/Stop)
- N-channel MOSFETs (one per solenoid)
- 5V Solenoid actuators
- Optional: Piezo sensors for timing analysis

### Solenoid Circuit

Each solenoid channel requires:

- 5V Solenoid (1A peak current)
- IRLZ44N N-channel MOSFET
- 1N4007 Flyback diode
- 470µF electrolytic capacitor
- 100nF ceramic capacitor
- 10kΩ pull-down resistor

```
                  5V
                   │
                   ├──┬──────┐
                   │  │      │
               100nF │  470µF│
                   │  │      │
                   │  │      │
         ┌─────────┴──┴──────┤
         │      SOLENOID     │
         │                   │
         └──────────┬────────┘
                    │    ┌─── 1N4007
                    ├────┤
                    │    └───┐
                    │        │
ESP32 ──┐          │        │
      10kΩ         │        │
         └───────┤ │        │
              IRLZ44N       │
                    │        │
                    └────────┘
                    GND
```

The parallel capacitors help manage current spikes:

- 470µF electrolytic: Main power reservoir
- 100nF ceramic: High-frequency transient suppression
- Flyback diode: Protects against back-EMF
- Pull-down resistor: Ensures MOSFET off state

### Piezo Tap Tempo Circuit

Components per tap sensor:

- Piezo disc sensor
- 2x 20kΩ resistors (voltage divider)
- 100nF capacitor (debounce)

```
           3.3V
            │
            │
         20kΩ
            │
ESP32 ──────┼────┬───── PIEZO
            │    │
         20kΩ  100nF
            │    │
           GND  GND
```

The voltage divider centers the piezo output around 1.65V, allowing both positive and negative peaks to be detected.

### Pin Assignments

```cpp
// Display
#define DISPLAY_SDA 21
#define DISPLAY_SCL 22

// Controls
#define ENCODER_A 17
#define ENCODER_B 18
#define ENCODER_BTN 16
#define BTN_START 26
#define BTN_STOP 25

// Per Metronome Channel
#define SOLENOID_PIN_1 13  // Additional pins: 12, 14 for more channels

// Tap tempo
#define PIEZO_PIN_1 32
#define PIEZO_THRESHOLD 1000  // ADC raw value threshold
```

## Software Architecture

### Global State Management

```cpp
struct GlobalState {
    uint16_t masterBPM;
    float bpmMultiplier;     // 0.5, 1.0, 2.0
    uint8_t activeChannel;   // Currently selected channel
    uint8_t editLevel;       // 0: global, 1: channel, 2: pattern
    bool isPlaying;
    TapTempo tapTempo;

    vector<MetronomeChannel*> channels;
    float globalProgress;    // LCM cycle progress

    void update();          // Main timing update
    void draw();            // Render complete UI
    uint32_t getLCMPeriod(); // Calculate global cycle length
};
```

### Navigation Hierarchy

1. **Global Level** (editLevel = 0)

   - Master BPM control (20-500)
   - BPM Multiplier selection (½×, 1×, 2×)
   - Active channel selection
   - Controls:
     - Rotate: Select parameter
     - Push: Enter edit mode
     - Long push on BPM: Enter tap tempo mode
     - Start/Stop: Global playback

2. **Channel Level** (editLevel = 1)

   - Beats per bar (1-16)
   - Pattern selection
   - Channel enable/disable
   - Controls:
     - Rotate: Select parameter
     - Push: Enter edit mode
     - Long push: Return to global level

3. **Pattern Level** (editLevel = 2)
   - Step-by-step beat editing
   - Controls:
     - Rotate: Move between steps
     - Push: Toggle beat state (Empty → Normal → Accent)
     - Long push: Generate Euclidean pattern
     - First step + push: Exit to channel level

### Display Management

```cpp
class Display {
    private:
        SH1106Wire* oled;
        uint8_t layout[MAX_CHANNELS];  // Y-positions for channels

    public:
        void drawGlobalBar(float progress);
        void drawChannel(MetronomeChannel* ch, uint8_t y);
        void drawBeatIndicator(uint8_t x, uint8_t y, BeatType type);
        void drawFlash(uint8_t channel, BeatType type);
};

// Visual constants
#define GLOBAL_BAR_HEIGHT 2
#define CHANNEL_SPACING 30
#define FLASH_RADIUS 3
#define FLASH_DURATION_MS 50
```

### Screen Layout Management

```cpp
struct ScreenRegion {
    uint8_t x, y, w, h;
    bool isActive;

    void draw();
    bool contains(uint8_t px, uint8_t py);
};

class LayoutManager {
    private:
        vector<ScreenRegion> regions;

    public:
        void addChannel(uint8_t index);
        void removeChannel(uint8_t index);
        void recalculateLayout();
        ScreenRegion* getRegion(uint8_t channel);
};
```

### MetronomeChannel Class

Core class representing single metronome track:

```cpp
class MetronomeChannel {
  private:
    uint8_t id;                // Channel ID (1-3)
    uint16_t bpm;             // 20-500
    uint8_t beatsPerBar;      // 1-16
    uint8_t subdivision;      // 2,4,8 (half, quarter, eighth)
    vector<BeatType> pattern; // Beat types sequence
    bool enabled;             // On/Off state
    uint8_t solenoidPin;
    uint8_t piezoPin;
    float progress;           // 0.0 to 1.0

  public:
    // Core methods
    void update();           // Process timing
    void trigger();          // Actuate solenoid
    void draw(uint8_t y);    // Render channel UI
    void drawProgress(uint8_t y); // Render progress bar

    // Parameter setters with validation
    bool setBPM(uint16_t bpm);
    bool setBeats(uint8_t beats);
    bool setPattern(uint16_t pattern);
    bool setSubdivision(uint8_t subdiv);

    // Pattern editing methods
    void toggleBeat(uint8_t step);
    void generateEuclidean(uint8_t active);
    float getProgress() const;
    uint32_t getCyclePeriod() const;
};
```

### MetronomeChannel Usage

```cpp
class MetronomeEngine {
    private:
        GlobalState state;
        Display display;
        LayoutManager layout;

    public:
        void addChannel() {
            auto ch = new MetronomeChannel();
            state.channels.push_back(ch);
            layout.addChannel(state.channels.size() - 1);
        }

        void update() {
            state.update();

            // Process each channel
            for(auto ch : state.channels) {
                if(ch->needsUpdate()) {
                    ch->trigger();
                    display.drawFlash(ch->getId(), ch->getCurrentBeat());
                }
            }

            // Update visuals
            display.drawGlobalBar(state.globalProgress);
            for(auto ch : state.channels) {
                auto region = layout.getRegion(ch->getId());
                display.drawChannel(ch, region->y);
            }
        }
};
```

### Timing and Synchronization

```cpp
class TimingManager {
    private:
        hw_timer_t* timer;
        volatile SemaphoreHandle_t timerSemaphore;

    public:
        void setupTimer(uint32_t microseconds);
        void onTick();  // ISR callback
        bool waitTick(); // Returns true when tick occurred
};
```

### Display Layout

Top 2 pixels are used to display the global progress bar of the beats going through a Least Common Denominator cycle for all the channels combined. If the number of such beats is higher than the number of available pixels - estimate a smooth progressbar instead.

Screen divided into 30-pixel high sections per channel, with progress indicators:

```
▲▲▲□□□□□□□□□
┌──────────────────┐
│ CH1 BPM: 120   ◐│  4-33px
│ 4/4   Pat: 1   ▮│
│ ▲ □ □ □        │
│ ▔▔▔▔▔▔░░░░░░░░ │
├──────────────────┤
│ CH2 BPM: 180   □│  34-63px
│ 3/4   Pat: 2   ▮│
│ ■ □ ■          │
│ ▔▔░░░░░░░░░░░░ │
└──────────────────┘

Legend:
▮ - Accent beat
□ - Normal beat
  - Empty step
▲ - Selected step in edit mode
▔ - Progress bar
◐ - Global cycle progress
```

```
┌──────────────────┐
│120 BPM x1    5/12│
│|**--------------|│
│04|**------------|│
│[ X ][ X ][ ][ x ]│
│03|**------------|│
│[  X  ][   ][    ]│
└──────────────────┘
```

Navigation hierarchy

1.  Global row BPM, multiplier, click indicator and LCD number (current and total)
2.  global progress bar
3.  1st channel block
    1.  1st row
        1.  Click indicator block with the bar length of the channel inside - briefly flashes white fill for accent, outline for weak beat and small dot for silent beat. On enc_push enable/disable the channel. On enc_push hold reset the pattern to Euclidean with the same number of active steps as at the moment of long press.
        2.  channel progress bar / active step indicator for step edit mode
    2.  First beat - always active, on enc_push we go down the pattern editing mode - instead of progress bar we can display the pointer that will show the step we are editing now. Clicking the steps toggles their state immediately. Choosing the first step and clicking it gets us out back to the channel level.
4.  2nd channel block - same as 1st.

So on main level of navigation we cycle through: BPM, multiplier, ch1 length, ch1 pattern, ch2 length, ch2 pattern.

### Beat Types

- **Accent (Strong)**: 7ms solenoid activation
- **Normal (Weak)**: 5ms solenoid activation
- **Empty**: No activation

```cpp
enum BeatType {
    EMPTY = 0,
    NORMAL = 1,
    ACCENT = 2
};
```

### Parameter Ranges

```cpp
// Timing
#define MIN_BPM 20
#define MAX_BPM 500
#define MIN_BEATS 1
#define MAX_BEATS 16
#define MIN_SUBDIVISION 2
#define MAX_SUBDIVISION 8

// Hardware
#define NORMAL_HIT_MS 5
#define ACCENT_HIT_MS 7
#define MIN_TRIGGER_INTERVAL_MS 100

// Display
#define CHANNEL_HEIGHT 32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
```

## User Interface

### Navigation & Control

1. **Basic Navigation**

   - Encoder rotation: Navigate parameters/channels
   - Encoder push: Enter/exit edit mode
   - Start button: Begin playback
   - Stop button: Stop playback

2. **Pattern Editing**

   - Short encoder push on pattern: Enter step editing mode
   - In step editing mode:
     - Rotate encoder: Move step selection
     - Push encoder: Toggle beat type (Empty -> Normal -> Accent)
     - Select first step + push: Exit pattern editing
   - Long encoder push (2s) on pattern: Generate Euclidean rhythm
     - Additional rotation: Adjust active steps count
     - Short push: Confirm pattern
     - Long push: Cancel

3. **Visual Feedback**
   - Arrow indicator (▲) shows selected step in edit mode
   - Pattern progress bar shows current position in pattern
   - Global progress bar on the top shows current position in the global cycle of Least Common Denominator beats
   - Beat types displayed with different symbols
   - Pattern visualization updates in real-time. Steps are evenly spaced between sides of the screen and can fit it with 1-16 steps.

### Tap Tempo Control

- When BPM selected and playback stopped:
  - Tap the device body near piezo sensor
  - Maintain steady rhythm for 2-4 taps
  - BPM updates after each valid tap
  - Display shows "TAP" indicator when in tap mode
  - Timeout resets tap sequence
  - Rotating encoder exits tap mode

### Synchronization

- Global cycle calculated as LCM of all active channel periods
- Circular progress indicator shows position in global cycle
- Each channel maintains independent linear progress bar
- All channels reset on global cycle completion

## Implementation Priorities

1. Single Channel Core

- [x] Basic timing engine
- [x] Pattern generation
- [x] Solenoid control
- [x] Display rendering

2. Multi-Channel Features

- [ ] Channel class implementation
- [ ] Synchronized timing
- [ ] Channel management
- [ ] UI adaptation

3. Advanced Features

- [ ] Tap tempo
- [ ] MIDI CLOCK sync in and out
- [ ] BLE MIDI Wireless control

## Code Structure

```cpp
src/
  ├── main.cpp              // Program entry, setup, loop
  ├── config.h              // Constants and configurations
  ├── MetronomeChannel.h    // Channel class definition
  ├── MetronomeChannel.cpp  // Channel implementation
  ├── Display.h            // Display management
  ├── Display.cpp          // UI rendering
  ├── Controls.h           // Input handling
  ├── Controls.cpp         // Encoder and button logic
```

## Notes for Implementation

1. Use hardware interrupts for encoder
2. Implement non-blocking timing
3. Optimize display updates
4. Add error checking
5. Include parameter validation
6. Design for extensibility
7. Document public interfaces
8. Add unit tests

### Tap Tempo Implementation

```cpp
#define TAP_TIMEOUT_MS 2000      // Reset tap sequence after 2s
#define MIN_TAP_INTERVAL_MS 200  // Debounce and max BPM limit
#define TAP_BUFFER_SIZE 4        // Average last 4 taps

class TapTempo {
    private:
        uint32_t lastTap;
        uint32_t intervals[TAP_BUFFER_SIZE];
        uint8_t tapCount;

    public:
        bool addTap();           // Returns true if BPM updated
        uint16_t getBPM();       // Current averaged BPM
        void reset();            // Clear tap sequence
};
```

Tap tempo is available when:

1. BPM parameter is selected
2. Playback is stopped
3. Last tap was within TAP_TIMEOUT_MS

Algorithm:

1. Store intervals between last TAP_BUFFER_SIZE taps
2. Calculate moving average of intervals
3. Convert average interval to BPM
4. Reset sequence if timeout exceeded
5. Ignore taps faster than MIN_TAP_INTERVAL_MS
6. Round result to nearest integer BPM
