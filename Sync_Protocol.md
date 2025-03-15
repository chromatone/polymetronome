# ESP32 Metronome Wireless Sync Protocol

## Overview

This document describes the wireless synchronization protocol used between ESP32 devices in the metronome system. The protocol uses ESP-NOW for low-latency communication and implements a leader-follower architecture with automatic clock synchronization based on musical timing events and latency-aware phase-locked loop (PLL) synchronization.

## Protocol Architecture

### Transport Layer
- **Protocol**: ESP-NOW
- **Mode**: Broadcast (one-to-many)
- **Maximum Packet Size**: 250 bytes
- **Typical Latency**: < 5ms
- **Latency Tracking**: Rolling average of 8 samples
- **Clock Drift Correction**: Continuous PLL-based adjustment

### Musical Time Integration

The protocol is designed around musical timing events rather than arbitrary time intervals:

1. **CLOCK messages**: Sent at SYNC24 intervals (24 pulses per quarter note)
   - Includes microsecond-precision timestamps
   - Adaptive rate based on tempo (reduced rate at high BPM)
   - Used for latency measurement and drift correction

2. **BEAT messages**: Sent on each quarter note boundary
   - Contains current tempo and beat position
   - Used for structural synchronization

3. **BAR messages**: Sent at the beginning of each measure/pattern cycle
   - Provides pattern and measure alignment
   - Ensures long-term structural sync

### Message Structure

All messages follow this common structure:
```cpp
typedef struct {
  MessageType type;           // Message type (1 byte)
  uint8_t deviceID[6];        // MAC address of sender (6 bytes)
  uint32_t sequenceNum;       // Sequence number (4 bytes)
  uint8_t priority;           // Device priority (1 byte)
  uint64_t timestamp;         // Microsecond-precision timestamp (8 bytes)
  union {
    // Message-specific data
    ClockData clock;
    BeatData beat;
    BarData bar;
    ControlData control;
    PatternData pattern;
  } data;
} SyncMessage;
```

### Message Types

1. **CLOCK (MSG_CLOCK = 0)**
   ```cpp
   struct {
     uint8_t isLeader;       // Is this device the sync leader
     uint32_t clockTick;     // Current SYNC24 tick count
     uint8_t reserved[3];    // Reserved for future use
   } clock;
   ```
   - Sent on each SYNC24 callback (24 times per quarter note)
   - Essential for tight synchronization of clock timing
   - Minimal payload to reduce bandwidth

2. **BEAT (MSG_BEAT = 1)**
   ```cpp
   struct {
     float bpm;              // Current BPM (4 bytes)
     uint8_t beatPosition;   // Current beat within bar
     uint8_t multiplierIdx;  // Tempo multiplier index
     uint8_t reserved[2];    // Reserved
   } beat;
   ```
   - Sent on each quarter note boundary
   - Contains tempo and beat position information
   - Serves as mid-level sync reference

3. **BAR (MSG_BAR = 2)**
   ```cpp
   struct {
     uint8_t channelCount;     // Number of active channels
     uint8_t activePattern;    // Current pattern ID
     uint16_t patternLength;   // Length of pattern in beats
     uint32_t channelMask;     // Bit mask of channel states
     uint32_t globalBar;       // Global bar counter
   } bar;
   ```
   - Sent at the beginning of each measure/pattern cycle
   - Contains pattern information
   - Used for higher-level musical structure sync

4. **CONTROL (MSG_CONTROL = 3)**
   ```cpp
   struct {
     uint8_t command;        // Command code
     uint8_t param1;         // Parameter 1
     uint8_t param2;         // Parameter 2
     uint8_t param3;         // Parameter 3
     uint32_t value;         // Command value
   } control;
   ```
   - Sent on transport state changes
   - Commands: START(1), STOP(2), PAUSE(3), RESET(4)
   - Not tied to musical timing events

5. **PATTERN (MSG_PATTERN = 4)**
   ```cpp
   struct {
     uint8_t channelId;      // Channel identifier
     uint8_t barLength;      // Pattern length in beats
     uint16_t pattern;       // Bit pattern (16 steps max)
     uint8_t currentBeat;    // Current active step
     uint8_t enabled;        // Channel state
     uint8_t reserved[2];    // Reserved
   } pattern;
   ```
   - Sent when patterns change or upon request
   - Contains complete pattern definition for a channel
   - Allows followers to precisely recreate patterns

## Enhanced Clock Synchronization

The system uses a sophisticated multi-layered approach for clock synchronization:

1. **Latency-Aware PLL**
   - Each follower maintains a rolling buffer of latency measurements
   - Message timestamps use microsecond precision
   - Network delay is measured and compensated for
   - Drift correction is applied gradually to maintain stability

2. **Predictive Timing**
   ```cpp
   struct TimingState {
     uint32_t latencyBuffer[8];    // Rolling latency measurements
     uint32_t averageLatency;      // Moving average of latency
     uint32_t lastReceivedTick;    // Last received clock tick
     uint32_t predictedNextTick;   // Predicted next tick number
     float driftCorrection;        // Current drift correction factor
   };
   ```

3. **Phase Error Correction**
   - Phase errors are measured between predicted and actual timing
   - Small errors (< 100μs) are corrected through tempo adjustment
   - Large errors trigger PLL resync
   - Correction factors are limited to prevent instability

4. **Adaptive Clock Rate**
   - CLOCK message rate adapts to tempo:
     * ≤ 120 BPM: Every SYNC24 pulse
     * ≤ 240 BPM: Every other pulse
     * > 240 BPM: Every fourth pulse
   - Prevents network congestion at high tempos
   - Maintains sync accuracy at all tempos

## Implementation Details

### Leader Device

1. **Initialization**
```cpp
WirelessSync sync;
sync.init();
sync.setAsLeader(true);
uClock.setOnSync24(sync.onSync24);
uClock.setOnPPQN(sync.onPPQN);
uClock.setOnStep(sync.onStep);
```

2. **uClock Callbacks**
```cpp
void onSync24(uint32_t tick) {
  // Send CLOCK message on each SYNC24 pulse
  sync.sendClock(tick);
}

void onPPQN(uint32_t tick) {
  // Process PPQN ticks for internal timing
  if (tick % 96 == 0) {
    // Every quarter note, send BEAT message
    sync.sendBeat(tick/96);
  }
}

void onStep(uint32_t step) {
  // At the beginning of each pattern/measure
  if (step % patternLength == 0) {
    // Send BAR message
    sync.sendBar(step/patternLength);
  }
}
```

3. **Message Broadcasting**
- Leverages uClock callbacks for timing-accurate message broadcasting
- Messages are sent at musically relevant moments
- Pattern changes are transmitted immediately when they occur

### Follower Device

1. **Initialization**
```cpp
uClock.init();
uClock.setMode(uClock.EXTERNAL_CLOCK);
uClock.setOnPPQN(onClockPulse);
uClock.setPPQN(uClock.PPQN_96);
```

2. **Message Processing**
```cpp
void handleClock(const SyncMessage &msg) {
  // Call uClock.clockMe() to sync timing
  uClock.clockMe();
}

void handleBeat(const SyncMessage &msg) {
  // Update BPM and beat position
  currentBpm = msg.data.beat.bpm;
  currentBeat = msg.data.beat.beatPosition;
}

void handleBar(const SyncMessage &msg) {
  // Update pattern positions and structure
  // Reset beat counters to align with measure
  // ...
}
```

## Flow of Synchronization

1. **Continuous Clock Alignment**
   - Leader sends CLOCK messages at each SYNC24 event (24 per quarter note)
   - Followers call `uClock.clockMe()` to maintain tight sync
   - PLL adjusts timing to match leader

2. **Beat Confirmation**
   - Leader sends BEAT messages at quarter note boundaries
   - Contains current tempo and beat position
   - Followers adjust local beat counters if drift is detected

3. **Structural Alignment**
   - Leader sends BAR messages at measure/pattern boundaries
   - Ensures patterns are perfectly aligned with musical structure
   - Allows for longer-term structural synchronization

4. **Pattern Distribution**
   - Complete pattern data sent when patterns change
   - Followers store patterns for local playback
   - Ensures visual and beat patterns are identical across devices

## Error Handling and Recovery

1. **Connection Loss**
   - Followers monitor CLOCK message frequency
   - Connection loss detected after 2000ms timeout
   - Local clock continues at last known good tempo
   - Smooth recovery when connection resumes

2. **Drift Management**
   - Continuous monitoring of clock drift
   - Gradual tempo adjustments (max 0.01% per correction)
   - Hard sync only when phase error exceeds half tick interval

3. **Message Validation**
   - Sequence numbers track message order
   - Timestamp validation prevents out-of-order processing
   - MAC address filtering prevents self-messages

## Best Practices

1. **Timing-Critical Processing**
   - Keep CLOCK message handling as lightweight as possible
   - Process within interrupt context if necessary
   - Minimize processing in tight timing loops

2. **Rate Management**
   - Adaptive CLOCK message rate based on tempo
   - Prevents network congestion at high tempos
   - Maintains sync accuracy across tempo range

3. **Visual Feedback**
   - Update visuals on BEAT messages rather than CLOCK
   - More efficient CPU and power usage
   - Maintains visual synchronization

## Testing and Validation

1. **Latency Measurement**
   - Monitor rolling latency buffer
   - Track average and peak latencies
   - Verify stability of latency measurements

2. **Drift Analysis**
   - Monitor drift correction factor
   - Verify stability at different tempos
   - Test recovery from induced drift

3. **Sync Accuracy**
   - Measure phase error over time
   - Verify stability of tempo adjustments
   - Test with varying network conditions

4. **Recovery Testing**
   - Simulate connection loss scenarios
   - Measure resync time and accuracy
   - Verify smooth visual transitions 