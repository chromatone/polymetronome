#pragma once

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <uClock.h>
#include "MetronomeState.h"

// Message types for our sync protocol
typedef enum {
  MSG_CLOCK = 0,
  MSG_BEAT = 1,
  MSG_BAR = 2,
  MSG_CONTROL = 3,
  MSG_PATTERN = 4
} MessageType;

// Main message structure for ESP-NOW sync
typedef struct {
  MessageType type;           // Message type (1 byte)
  uint8_t deviceID[6];        // MAC address of sender (6 bytes)
  uint32_t sequenceNum;       // Sequence number (4 bytes)
  uint8_t priority;           // Device priority (1 byte)
  uint64_t timestamp;         // Global timestamp (8 bytes)
  
  // Message-specific data
  union {
    // CLOCK data (SYNC24 pulse)
    struct {
      uint8_t isLeader;       // Is this device the sync leader (1 byte)
      uint32_t clockTick;     // Current SYNC24 tick count (4 bytes)
      uint8_t reserved[3];    // Reserved for future use (3 bytes)
    } clock;
    
    // BEAT data (quarter note)
    struct {
      float bpm;              // Current BPM (4 bytes)
      uint8_t beatPosition;   // Current beat within bar (1 byte)
      uint8_t multiplierIdx;  // Current multiplier index (1 byte)
      uint8_t reserved[2];    // Reserved (2 bytes)
    } beat;
    
    // BAR data (measure/pattern start)
    struct {
      uint8_t channelCount;    // Number of active channels (1 byte)
      uint8_t activePattern;   // Current pattern ID (1 byte)
      uint16_t patternLength;  // Length of pattern in beats (2 bytes)
      uint32_t channelMask;    // Bit mask of channel states (4 bytes)
      uint32_t globalBar;      // Global bar counter (4 bytes)
    } bar;
    
    // PATTERN data (pattern definition)
    struct {
      uint8_t channelId;      // Channel ID (1 byte)
      uint8_t barLength;      // Pattern length in steps (1 byte)
      uint16_t pattern;       // Bit pattern (16 bits = 16 steps max) (2 bytes)
      uint8_t currentBeat;    // Current active step (1 byte)
      uint8_t enabled;        // Channel enabled state (1 byte)
      uint8_t reserved[2];    // Reserved (2 bytes)
    } pattern;
    
    // CONTROL data
    struct {
      uint8_t command;        // Command code (1 byte)
      uint8_t param1;         // Parameter 1 (1 byte)
      uint8_t param2;         // Parameter 2 (1 byte)
      uint8_t param3;         // Parameter 3 (1 byte)
      uint32_t value;         // Command value (4 bytes)
    } control;
  } data;
} SyncMessage;

// Control commands
enum ControlCommand {
  CMD_START = 1,
  CMD_STOP = 2,
  CMD_PAUSE = 3,
  CMD_RESET = 4
};

class WirelessSync {
private:
  uint8_t _broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  uint8_t _deviceID[6];
  uint32_t _sequenceNum;
  uint8_t _priority;
  bool _isLeader;
  bool _initialized;
  
  // Track which sync messages have been sent
  uint32_t _lastSync24Tick;
  uint32_t _lastQuarterNote;
  uint32_t _lastBarStart;
  bool _patternChanged;
  
  // Leader selection
  uint32_t _lastLeaderHeartbeat;
  uint32_t _leaderTimeoutMs;
  bool _leaderNegotiationActive;
  uint8_t _currentLeaderID[6];
  uint8_t _highestPriorityDevice[6];
  uint8_t _highestPrioritySeen;
  
  // Latency tracking
  uint32_t _latencyBuffer[8];  // Rolling buffer of latency measurements
  uint8_t _latencyBufferIndex;
  uint32_t _averageLatency;    // Moving average of latency
  uint64_t _lastSendTime;      // Timestamp of last sent message
  
  // Enhanced sync tracking
  uint32_t _lastReceivedTick;
  uint32_t _predictedNextTick;
  float _driftCorrection;
  
  // State reference for pattern updates
  MetronomeState* _state;
  
  // Helper functions for pattern length calculations
  uint16_t lcm(uint16_t a, uint16_t b);
  uint16_t gcd(uint16_t a, uint16_t b);
  
  // Callback functions
  static void onDataReceived(const uint8_t *mac, const uint8_t *data, int len);
  
  // Send a message
  void sendMessage(SyncMessage &msg);
  
  // Leader selection methods
  void startLeaderNegotiation();
  void processLeaderSelection(const SyncMessage &msg);
  bool isLeaderTimedOut();
  bool isHigherPriority(const uint8_t *deviceID, uint8_t priority);
  
  void updateLatency(uint64_t sendTime);
  uint32_t getAverageLatency() const;
  void predictNextTick(uint32_t currentTick, uint64_t timestamp);
  
public:
  // Constructor initialization
  WirelessSync() : 
      _sequenceNum(0),
      _priority(1),
      _isLeader(false),
      _initialized(false),
      _lastSync24Tick(0),
      _lastQuarterNote(0),
      _lastBarStart(0),
      _patternChanged(false),
      _leaderTimeoutMs(3000),
      _lastLeaderHeartbeat(0),
      _leaderNegotiationActive(false),
      _highestPrioritySeen(0),
      _latencyBufferIndex(0),
      _averageLatency(0),
      _lastSendTime(0),
      _lastReceivedTick(0),
      _predictedNextTick(0),
      _driftCorrection(1.0f),
      _state(nullptr)
  {
      memset(_currentLeaderID, 0, sizeof(_currentLeaderID));
      memset(_highestPriorityDevice, 0, sizeof(_highestPriorityDevice));
      memset(_latencyBuffer, 0, sizeof(_latencyBuffer));
  }
  
  // Initialize ESP-NOW
  bool init();
  
  // Check if wireless sync is initialized
  bool isInitialized() const { return _initialized; }
  
  // Set device as leader
  void setAsLeader(bool isLeader);
  
  // Set device priority (higher value = higher priority)
  void setPriority(uint8_t priority);
  
  // Check if this device is currently the leader
  bool isLeader() const;
  
  // Leader negotiation
  void negotiateLeadership();
  void checkLeaderStatus();
  
  // uClock callback handlers (to be connected to uClock callbacks)
  void onSync24(uint32_t tick);
  void onPPQN(uint32_t tick, MetronomeState &state);
  void onStep(uint32_t step, MetronomeState &state);
  
  // Send musical-timing based messages
  void sendClock(uint32_t tick);
  void sendBeat(uint32_t beat, MetronomeState &state);
  void sendBar(uint32_t bar, MetronomeState &state);
  
  // Send pattern definition
  void sendPattern(MetronomeState &state, uint8_t channelId);
  
  // Send control message
  void sendControl(uint8_t command, uint32_t value = 0);
  
  // Handle pattern changes from the metronome state
  void notifyPatternChanged(uint8_t channelId);
  
  // Update function to be called in loop (for pattern change detection)
  void update(MetronomeState &state);
  
  // Enhanced sync methods
  uint32_t getLatency() const { return _averageLatency; }
  float getDriftCorrection() const { return _driftCorrection; }
}; 