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
- Wireless sync via ESP-NOW for multiple devices

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
- [x] Settings storage
- [x] Wireless control

3. Hardware Improvements

- [ ] PCB design
- [ ] Case design
- [ ] Power supply optimization
- [x] Multiple solenoid support
- [x] LED indicators

4. Modular system:

- [ ] Connectors
- [x] Solenoids
- [ ] Piezo buzzers
- [ ] Speakers/amplifiers
- [x] LED arrays/strips
- [x] LCD displays
- [ ] MIDI interface modules
- [x] Rotary encoders
- [x] Buttons/switches
- [ ] Potentiometers
- [ ] Accelerometers
- [x] Wireless modules (WiFi/BLE)
- [ ] SD card readers
- [ ] External DACs
- [ ] CV/Gate outputs
- [ ] USB interfaces

## Wireless Sync System

The metronome now includes a wireless synchronization system using ESP-NOW, allowing multiple devices to stay in perfect sync:

- **Leader-Follower Architecture**: One metronome acts as the leader, broadcasting timing and pattern information
- **Low Latency**: ESP-NOW provides sub-5ms latency for tight synchronization
- **Robust Protocol**: Custom protocol with heartbeats, tempo, pattern, and control messages
- **Auto-Recovery**: Followers automatically reconnect if connection is lost
- **Expandable**: Support for LED strips, additional solenoids, and other output devices

### LED Receiver

A companion project is included for an ESP32-controlled LED strip that syncs with the metronome:

- Receives wireless sync data from the metronome
- Visualizes beats with configurable colors for each channel
- Automatically follows pattern changes
- See the [LED Receiver README](led_receiver/README.md) for details
