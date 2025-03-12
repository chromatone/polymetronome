# ESP32 Polymetronome Project

## Core Concept

A modular multi-track hardware metronome system capable of running multiple independent rhythm patterns synchronized to a master tempo. Built on ESP32 with OLED display and precise solenoid actuation.

## Documentation

- [Hardware Documentation](docs/hardware.md) - Circuit diagrams and component specifications
- [Software Architecture](docs/software.md) - Code structure and implementation details
- [Screen Layout](docs/screen.md) - UI design and display management

## Current Features

- Two independent metronome channels
- 20-500 BPM range with multipliers (1/4×, 1/3×, 1/2×, 1×, 2×, 3×, 4×)
- Pattern editing with up to 16 steps per channel
- Global tempo synchronization
- Real-time visual feedback
- Solenoid actuation with accent support

## Implementation Status

1. Core Functionality

- [x] Basic timing engine
- [x] Pattern storage and playback
- [x] Solenoid control
- [x] Display rendering
- [x] Encoder input handling
- [x] Multi-channel synchronization

2. Enhanced Features

- [ ] Tap tempo input
- [ ] Pattern templates
- [ ] MIDI Clock sync
- [ ] Settings storage
- [ ] Wireless control

3. Hardware Improvements

- [ ] PCB design
- [ ] Case design
- [ ] Power supply optimization
- [ ] Multiple solenoid support
- [ ] LED indicators

4. Modular system:

- [ ] Connectors
- [ ] Solenoids
- [ ] Piezo buzzers
- [ ] Speakers/amplifiers
- [ ] LED arrays/strips
- [ ] LCD displays
- [ ] MIDI interface modules
- [ ] Rotary encoders
- [ ] Buttons/switches
- [ ] Potentiometers
- [ ] Accelerometers
- [ ] Wireless modules (WiFi/BLE)
- [ ] SD card readers
- [ ] External DACs
- [ ] CV/Gate outputs
- [ ] USB interfaces
