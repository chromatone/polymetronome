# Polyrhythmic Hardware Metronome

A versatile hardware metronome capable of generating complex rhythmic patterns with visual feedback. Built on ESP32 platform with a focus on musical experimentation and live performance use.

## Components

- **MCU**: ESP32 Development Board
  - 240 MHz dual-core processor
  - Hardware interrupts for precise timing
- **Display**: SH1106 128x64 OLED Display
  - I2C interface
  - High contrast monochrome display
- **Controls**:
  - Rotary Encoder with push button (main control)
  - 2 auxiliary buttons (Start/Stop)
  - All inputs with hardware debouncing

## Features

### Current Implementation

- Base tempo control (10-500 BPM)
- Pattern length control (1-16 beats)
- Binary pattern generation (combinations of accented/silent beats)
- Visual feedback:
  - Current tempo display
  - Pattern length indicator
  - Pattern number display
  - Beat grid visualization
  - Current position indicator
  - Active beat highlighting
- Parameter editing system:
  - Menu navigation
  - Value adjustment with acceleration
  - Clear visual editing indicators
- Real-time pattern playback
  - Start/Stop control
  - Beat position tracking
  - Pattern repetition

### Architecture Decisions

1. **Timing System**:

   - Hardware interrupt-based encoder reading
   - Millisecond-precision beat timing
   - Pattern-independent base tempo

2. **User Interface**:

   - Three-parameter menu system
   - Combined navigation/editing mode
   - Visual feedback prioritization
   - Accumulator-based encoder sensitivity

3. **Pattern System**:
   - Binary pattern storage (16-bit)
   - First-beat-always-on constraint
   - Length-based pattern organization

## TODO List

### Musical Features

- [ ] Tap tempo functionality
- [ ] Swing timing adjustment
- [ ] Multiple pattern storage/recall
- [ ] Pattern chaining/sequencing
- [ ] Polyrhythm mode (multiple simultaneous patterns)
- [ ] MIDI sync (in/out)
- [ ] Variable accent levels (not just on/off)
- [ ] Tempo multiplication/division

### User Experience

- [ ] Pattern preview while editing
- [ ] Save/recall user presets
- [ ] Quick pattern presets (common rhythms)
- [ ] Visual metronome animations
- [ ] Battery level indicator
- [ ] Adjustable display brightness
- [ ] Menu system deep dive protection
- [ ] Calibration mode for encoder

### Technical Improvements

- [ ] Power management optimization
- [ ] EEPROM pattern storage
- [ ] More efficient display updates
- [ ] Timing drift compensation
- [ ] Hardware output for solenoid control
- [ ] Wireless control capabilities
- [ ] Multi-core task distribution
- [ ] Error handling and recovery

### Hardware Additions

- [ ] Solenoid driver circuit
- [ ] Audio output
- [ ] External sync connections
- [ ] Additional control inputs
- [ ] Battery power system
- [ ] Case design

## Contributing

This is an open-source project under active development. Contributions, suggestions, and feedback are welcome.

## License

[MIT License](LICENSE)
