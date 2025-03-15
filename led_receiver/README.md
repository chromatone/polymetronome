# ESP32 LED Strip Receiver

This project implements an ESP32-based LED strip controller that synchronizes with a metronome via ESP-NOW wireless communication using a musically-driven protocol.

## Features

- Receives wireless sync data from the metronome using ESP-NOW
- Uses a musically-driven protocol based on actual timing events rather than arbitrary timeouts
- Synchronizes LED patterns with metronome beats using uClock's external clock mode
- Supports multiple channels with different patterns
- Visual feedback for connection status and beat patterns
- Automatic recovery from connection loss
- Sub-5ms latency for tight synchronization
- Hierarchical musical timing sync (CLOCK → BEAT → BAR)

## Hardware Requirements

- ESP32 development board
- WS2812B LED strip (or similar FastLED-compatible strip)
- 5V power supply for the LED strip (adequate for number of LEDs)
- Connecting wires

## Wiring

- Connect the LED strip data pin to GPIO 4
- Connect the LED strip power to 5V and ground
- Ensure good power distribution for longer LED strips

## Dependencies

- Arduino framework
- ESP32 Arduino core
- FastLED library (v3.5.0 or later)
- uClock library (v1.0.1 or later)

## Setup

1. Connect the hardware as described above
2. Install required libraries using PlatformIO or Arduino IDE
3. Upload the code to your ESP32
4. Power on the metronome transmitter
5. The LED strip will automatically sync with the metronome

## Configuration

### LED Strip Settings
```cpp
#define LED_PIN     4  // Data pin connected to G4
#define NUM_LEDS    33 // Number of LEDs (from 40 to 72 = 33 LEDs)
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
```

### Visual Parameters
```cpp
// Duration of LED flash in milliseconds
uint32_t beatDuration = 100;

// Colors for each channel
CRGB channelColors[2] = {CRGB::Red, CRGB::Blue};
```

### Sync Parameters
```cpp
// Clock message timeout for connection monitoring (milliseconds)
#define CLOCK_TIMEOUT 500

// LED brightness (0-255)
#define LED_BRIGHTNESS 50
```

## Status Indicators

The first LED (index 0) shows the device status:
- **Dim Green**: Ready and waiting for connection
- **Blinking Red**: Connection lost to metronome
- **Off**: Part of the pattern display

## Channel Layout

The LED strip is divided into two sections for the two channels:
- Channel 1: LEDs 0 to (NUM_LEDS/2 - 1)
- Channel 2: LEDs (NUM_LEDS/2) to (NUM_LEDS - 1)

Each channel displays its pattern with:
- Independent colors
- Pattern-based flashing
- Fade-out effect for smoother visuals

## Musically-Driven Sync Protocol

This device implements the follower role in the sync protocol, which is based on musical timing events:

1. **CLOCK Messages**
   - Received at SYNC24 intervals (24 pulses per quarter note)
   - Used for precise timing synchronization
   - Triggers uClock.clockMe() for tight sync

2. **BEAT Messages**
   - Received on each quarter note boundary
   - Updates tempo and beat position
   - Triggers LED flashes at musically relevant times

3. **BAR Messages**
   - Received at the start of each measure/pattern
   - Ensures correct pattern alignment
   - Resets beat counters when needed

4. **PATTERN Messages**
   - Received when patterns change
   - Contains complete pattern definitions
   - Updates local pattern storage

5. **CONTROL Messages**
   - Commands for transport control (start/stop/etc.)
   - Not tied to musical timing events

## Error Recovery

The system uses a hierarchical approach to maintain sync:

1. **Fine Timing Sync**: CLOCK messages (24 PPQ)
2. **Beat-Level Sync**: BEAT messages (quarter notes)
3. **Bar-Level Sync**: BAR messages (pattern start)

If CLOCK messages stop arriving, the device detects connection loss and provides visual feedback. It will automatically recover when messages resume.

For detailed protocol documentation, see [Sync_Protocol.md](../Sync_Protocol.md)

## Troubleshooting

### Connection Issues
- If the first LED blinks red, check:
  - Metronome power and operation
  - Distance between devices
  - ESP-NOW initialization success
  - WiFi interference

### LED Issues
- If LEDs don't light up:
  - Verify power supply capacity
  - Check data pin connection
  - Confirm LED strip type settings
  - Test with basic FastLED example

### Sync Issues
- If beats are not aligned:
  - Verify both devices are using same PPQN setting
  - Check for WiFi interference
  - Monitor serial output for timing data
  - Reduce distance between devices

## Performance Notes

- CLOCK message handling is kept minimal for timing precision
- Visual updates happen on BEAT messages, not every CLOCK
- The system automatically adjusts message rates at high tempos
- ESP-NOW provides low-latency communication
- Multi-level sync ensures accuracy even if some messages are missed 