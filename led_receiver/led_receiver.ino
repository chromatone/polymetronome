#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <FastLED.h>
#include <uClock.h>

// LED strip configuration
#define LED_PIN     4
#define NUM_LEDS    33
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

// Timing constants
#define CLOCK_TIMEOUT 2000  // Connection timeout in ms

// Forward declarations
class LEDDisplay;
class SyncFollower;

// Message types from the protocol
typedef enum {
  MSG_CLOCK = 0,
  MSG_BEAT = 1,
  MSG_BAR = 2,
  MSG_CONTROL = 3,
  MSG_PATTERN = 4
} MessageType;

// Protocol message structures
typedef struct {
  MessageType type;           // Message type (1 byte)
  uint8_t deviceID[6];        // MAC address of sender (6 bytes)
  uint32_t sequenceNum;       // Sequence number (4 bytes)
  uint8_t priority;           // Device priority (1 byte)
  uint64_t timestamp;         // Global timestamp (8 bytes)
  
  union {
    struct {
      uint8_t isLeader;       // Is this device the sync leader
      uint32_t clockTick;     // Current SYNC24 tick count
      uint8_t reserved[3];    // Reserved for future use
    } clock;
    
    struct {
      float bpm;              // Current BPM
      uint8_t beatPosition;   // Current beat within bar
      uint8_t multiplierIdx;  // Current multiplier index
      uint8_t reserved[2];    // Reserved
    } beat;
    
    struct {
      uint8_t channelCount;     // Number of active channels
      uint8_t activePattern;    // Current pattern ID
      uint16_t patternLength;   // Length of pattern in beats
      uint32_t channelMask;     // Bit mask of channel states
      uint32_t globalBar;       // Global bar counter
    } bar;
    
    struct {
      uint8_t command;        // Command code
      uint8_t param1;         // Parameter 1
      uint8_t param2;         // Parameter 2
      uint8_t param3;         // Parameter 3
      uint32_t value;        // Command value
    } control;
    
    struct {
      uint8_t channelId;      // Channel ID
      uint8_t barLength;      // Pattern length in steps
      uint16_t pattern;       // Bit pattern (16 steps max)
      uint8_t currentBeat;    // Current active step
      uint8_t enabled;        // Channel enabled state
      uint8_t reserved[2];    // Reserved
    } pattern;
  } data;
} SyncMessage;

// LED Display class to handle visual feedback
class LEDDisplay {
private:
  CRGB* leds;
  uint8_t numLeds;
  uint8_t mainSection;    // Number of LEDs in main tempo section
  uint8_t channelSection; // Number of LEDs per channel section
  
  uint32_t lastBeatTime = 0;
  uint32_t beatDuration = 100; // Flash duration in ms
  
  // Enhanced pattern tracking
  struct ChannelState {
    bool enabled;
    uint8_t currentBeat;
    uint8_t barLength;
    uint16_t pattern;
    uint32_t lastUpdateTick;
  };
  ChannelState channels[2];
  
  // Global pattern sync
  uint32_t globalTick = 0;
  uint16_t totalPatternLength = 4;

public:
  LEDDisplay(CRGB* ledArray, uint8_t totalLeds) : leds(ledArray), numLeds(totalLeds) {
    // Divide strip into 3 sections (main + 2 channels)
    mainSection = numLeds / 3;
    channelSection = mainSection;
    
    // Initialize channel states
    for (int i = 0; i < 2; i++) {
      channels[i] = {false, 0, 4, 0, 0};
    }
    
    Serial.printf("LED sections: total=%d, main=%d, channel=%d\n",
                 numLeds, mainSection, channelSection);
  }

  void clear() {
    FastLED.clear();
  }

  void show() {
    FastLED.show();
  }

  void updateMainBeat() {
    uint32_t now = millis();
    if (now - lastBeatTime < beatDuration) {
      // Flash white on beat
      for (int i = 0; i < mainSection; i++) {
        leds[i] = CRGB::White;
      }
    } else {
      // Clear main section
      for (int i = 0; i < mainSection; i++) {
        leds[i] = CRGB::Black;
      }
    }
  }

  void updateChannels() {
    for (int i = 0; i < 2; i++) {
      if (channels[i].enabled) {
        int startLed = mainSection + (i * channelSection);
        int endLed = startLed + channelSection;
        
        // Calculate which LEDs should be lit based on current beat
        for (int led = startLed; led < endLed; led++) {
          bool isActive = false;
          
          // First beat is always active
          if (channels[i].currentBeat == 0) {
            isActive = true;
          } else {
            // Check pattern for other beats
            isActive = (channels[i].pattern >> (channels[i].currentBeat - 1)) & 1;
          }
          
          if (led - startLed == channels[i].currentBeat) {
            leds[led] = isActive ? CRGB::White : CRGB::Red;
          } else {
            leds[led] = CRGB::Black;
          }
        }
      }
    }
  }

  void onBeat(uint8_t beatPosition) {
    lastBeatTime = millis();
    globalTick = beatPosition;
    
    // Update channel beats based on their individual lengths
    for (int i = 0; i < 2; i++) {
      if (channels[i].enabled) {
        channels[i].currentBeat = beatPosition % channels[i].barLength;
      }
    }
  }

  void updateChannelPattern(uint8_t channel, uint8_t beat, uint8_t length, uint16_t pattern, bool enabled) {
    if (channel < 2) {
      channels[channel].currentBeat = beat;
      channels[channel].barLength = length;
      channels[channel].pattern = pattern;
      channels[channel].enabled = enabled;
      channels[channel].lastUpdateTick = globalTick;
    }
  }

  void setTotalPatternLength(uint16_t length) {
    totalPatternLength = length;
  }
};

// Sync Follower class to handle protocol implementation
class SyncFollower {
private:
  float currentBpm = 120.0;
  bool isRunning = false;
  bool connectionLost = false;
  uint64_t lastClockTime = 0;
  uint32_t lastClockTick = 0;
  
  LEDDisplay& display;
  
  struct TimingState {
    uint32_t latencyBuffer[8];
    uint32_t averageLatency;
    uint32_t lastReceivedTick;
    uint32_t predictedNextTick;
    float driftCorrection;
  } timing;

public:
  SyncFollower(LEDDisplay& disp) : display(disp) {
    timing.averageLatency = 0;
    timing.driftCorrection = 1.0;
  }

  void handleClock(const SyncMessage& msg) {
    lastClockTime = millis();
    lastClockTick = msg.data.clock.clockTick;
    
    if (connectionLost) {
      connectionLost = false;
      Serial.println("Connection restored");
    }

    if (!isRunning) return;

    // Calculate message latency
    uint32_t messageLatency = micros() - msg.timestamp;
    
    // Update timing state and apply PLL corrections
    updateTimingState(messageLatency, msg.timestamp);
    
    // Call uClock to maintain sync
    uClock.clockMe();
  }

  void handleBeat(const SyncMessage& msg) {
    // Update tempo if changed
    if (msg.data.beat.bpm != currentBpm) {
      currentBpm = msg.data.beat.bpm;
      uClock.setTempo(currentBpm);
    }

    // Update beat position and trigger visual indication
    display.onBeat(msg.data.beat.beatPosition);
    
    Serial.printf("Beat: position=%d/%d, bpm=%.1f\n", 
                 msg.data.beat.beatPosition,
                 msg.data.bar.patternLength,
                 currentBpm);
  }

  void handleBar(const SyncMessage& msg) {
    // Update total pattern length
    display.setTotalPatternLength(msg.data.bar.patternLength);
    
    Serial.printf("Bar: global=%lu, total_length=%d, channels=%d\n", 
                 msg.data.bar.globalBar,
                 msg.data.bar.patternLength,
                 msg.data.bar.channelCount);
  }

  void handlePattern(const SyncMessage& msg) {
    Serial.printf("Pattern: channel=%d, beat=%d/%d, enabled=%d\n",
                 msg.data.pattern.channelId,
                 msg.data.pattern.currentBeat,
                 msg.data.pattern.barLength,
                 msg.data.pattern.enabled);
                 
    if (msg.data.pattern.channelId < 2) {
      display.updateChannelPattern(
        msg.data.pattern.channelId,
        msg.data.pattern.currentBeat,
        msg.data.pattern.barLength,
        msg.data.pattern.pattern,
        msg.data.pattern.enabled
      );
    }
  }

  void handleControl(const SyncMessage& msg) {
    switch (msg.data.control.command) {
      case 1: // START
        isRunning = true;
        connectionLost = false;
        lastClockTime = millis();
        uClock.start();
        break;
      case 2: // STOP
        isRunning = false;
        uClock.stop();
        break;
      case 3: // PAUSE
        isRunning = false;
        uClock.stop();
        break;
      case 4: // RESET
        // Reset all channel beats
        for (uint8_t i = 0; i < 2; i++) {
          display.updateChannelPattern(i, 0, 4, 0, false);
        }
        break;
    }
  }

  void checkConnection() {
    if (isRunning && millis() - lastClockTime > CLOCK_TIMEOUT) {
      if (!connectionLost) {
        connectionLost = true;
        Serial.println("Connection to leader lost");
      }
    }
  }

  void update() {
    checkConnection();
    if (!connectionLost) {
      display.updateMainBeat();
      display.updateChannels();
      display.show();
    }
  }

private:
  void updateTimingState(uint32_t messageLatency, uint64_t messageTimestamp) {
    // Update latency buffer (simple rolling average)
    static uint8_t latencyIndex = 0;
    timing.latencyBuffer[latencyIndex] = messageLatency;
    latencyIndex = (latencyIndex + 1) % 8;

    // Calculate average latency
    uint32_t sum = 0;
    for (int i = 0; i < 8; i++) {
      sum += timing.latencyBuffer[i];
    }
    timing.averageLatency = sum / 8;

    // Calculate phase error and apply corrections
    uint32_t tickInterval = 60000000 / (currentBpm * 24); // microseconds per tick
    uint64_t predictedArrivalTime = messageTimestamp + timing.averageLatency;
    int32_t phaseError = micros() - predictedArrivalTime;

    // Apply tempo correction if phase error is significant
    if (abs(phaseError) > 100) { // More than 100μs error
      float tempoAdjustment = (phaseError > 0) ? 0.9999f : 1.0001f;
      float newTempo = currentBpm * tempoAdjustment;
      
      if (newTempo >= MIN_BPM && newTempo <= MAX_BPM) {
        currentBpm = newTempo;
        uClock.setTempo(newTempo);
      }
    }
  }
};

// Global instances
CRGB leds[NUM_LEDS];
LEDDisplay* display;
SyncFollower* follower;

// ESP-NOW callback
void onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(SyncMessage)) {
    Serial.printf("Invalid message size: %d\n", len);
    return;
  }

  const SyncMessage *msg = (const SyncMessage *)data;
  
  switch (msg->type) {
    case MSG_CLOCK:
      follower->handleClock(*msg);
      break;
    case MSG_BEAT:
      follower->handleBeat(*msg);
      break;
    case MSG_BAR:
      follower->handleBar(*msg);
      break;
    case MSG_PATTERN:
      follower->handlePattern(*msg);
      break;
    case MSG_CONTROL:
      follower->handleControl(*msg);
      break;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
         .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(50);
  FastLED.clear();
  FastLED.show();
  
  // Create display and follower instances
  display = new LEDDisplay(leds, NUM_LEDS);
  follower = new SyncFollower(*display);
  
  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register callback
  esp_now_register_recv_cb(onDataReceived);
  
  // Initialize uClock
  uClock.init();
  uClock.setMode(uClock.EXTERNAL_CLOCK);
  uClock.setPPQN(uClock.PPQN_96);
  uClock.setTempo(120.0);
  
  Serial.println("ESP-NOW Follower initialized");
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Show startup animation
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Green;
    FastLED.show();
    delay(20);
    leds[i] = CRGB::Black;
  }
  
  // Show ready state
  leds[0] = CRGB::Green;
  FastLED.show();
}

void loop() {
  follower->update();
  delay(1); // Small delay to prevent CPU hogging
} 