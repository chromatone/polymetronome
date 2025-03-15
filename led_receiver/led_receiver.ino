#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <FastLED.h>
#include <uClock.h>

// LED strip configuration
#define LED_PIN     4  // Data pin connected to G4
#define NUM_LEDS    33 // Number of LEDs (from 40 to 72 = 33 LEDs)
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

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
      uint8_t channelCount;     // Number of active channels (1 byte)
      uint8_t activePattern;    // Current pattern ID (1 byte)
      uint16_t patternLength;   // Length of pattern in beats (2 bytes)
      uint32_t channelMask;     // Bit mask of channel states (4 bytes)
      uint32_t globalBar;       // Global bar counter (4 bytes)
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

// Global variables
float currentBpm = 120.0;
bool isRunning = false;
uint64_t lastClockTime = 0;
uint64_t lastBeatTime = 0;
uint32_t beatDuration = 100; // Duration of LED flash in ms
uint16_t patterns[2] = {0xFFFF, 0xFFFF}; // Default patterns for both channels
uint8_t barLengths[2] = {4, 4}; // Default bar lengths
bool channelEnabled[2] = {true, true}; // Default channel states
uint8_t currentBeats[2] = {0, 0}; // Current beat position for each channel
uint32_t lastClockTick = 0; // Last received clock tick
CRGB channelColors[2] = {CRGB::Red, CRGB::Blue}; // Colors for each channel

// Connection monitoring
#define CLOCK_TIMEOUT 2000 // Increased timeout to 2 seconds
bool connectionLost = false;

// Function prototypes
void handleClock(const SyncMessage &msg);
void handleBeat(const SyncMessage &msg);
void handleBar(const SyncMessage &msg);
void handlePattern(const SyncMessage &msg);
void handleControl(const SyncMessage &msg);
void updateLEDs();
bool isBeatActive(uint8_t channel, uint8_t beat);

// Callback function for ESP-NOW
void onDataReceived(const uint8_t *mac, const uint8_t *data, int len) {
  static uint32_t lastMessageTime = 0;
  uint32_t now = millis();
  
  if (len != sizeof(SyncMessage)) {
    Serial.print("Received invalid data size: ");
    Serial.println(len);
    return;
  }
  
  const SyncMessage *msg = (const SyncMessage *)data;
  
  // Debug message timing
  if (msg->type != MSG_CLOCK || (lastClockTick % 24 == 0)) {  // Don't spam for clock messages
    Serial.print("Message received: type=");
    Serial.print(msg->type);
    Serial.print(", delta_t=");
    Serial.print(now - lastMessageTime);
    Serial.println("ms");
  }
  lastMessageTime = now;
  
  // Process message based on type
  switch (msg->type) {
    case MSG_CLOCK:
      handleClock(*msg);
      break;
    case MSG_BEAT:
      handleBeat(*msg);
      break;
    case MSG_BAR:
      handleBar(*msg);
      break;
    case MSG_PATTERN:
      handlePattern(*msg);
      break;
    case MSG_CONTROL:
      handleControl(*msg);
      break;
    default:
      Serial.println("Unknown message type");
      break;
  }
}

void handleClock(const SyncMessage &msg) {
  // Update connection monitoring
  lastClockTime = millis();
  
  // Track the clock tick
  lastClockTick = msg.data.clock.clockTick;
  
  // Calculate message latency
  uint32_t messageLatency = micros() - msg.timestamp;
  
  // Debug output every 24 ticks (once per quarter note)
  if (lastClockTick % 24 == 0) {
    Serial.print("CLOCK: tick=");
    Serial.print(lastClockTick);
    Serial.print(", latency=");
    Serial.print(messageLatency);
    Serial.print("us, isLeader=");
    Serial.print(msg.data.clock.isLeader);
    Serial.print(", running=");
    Serial.println(isRunning);
  }
  
  if (connectionLost) {
    connectionLost = false;
    Serial.println("Connection restored");
  }
  
  // Only process if running
  if (!isRunning) return;

  // Calculate tick interval based on current tempo
  uint32_t tickInterval = 60000000 / (currentBpm * 24); // microseconds per tick
  
  // Predict when this tick should have arrived without latency
  uint64_t predictedArrivalTime = msg.timestamp + messageLatency;
  
  // Calculate phase error between predicted and actual local clock
  uint64_t localClockTime = micros();
  int32_t phaseError = localClockTime - predictedArrivalTime;
  
  // Adjust local clock phase if error is significant
  if (abs(phaseError) > 100) { // More than 100μs error
    // Calculate tempo adjustment factor
    float tempoAdjustment = 1.0f;
    if (phaseError > 0) {
      // We're running fast, slow down slightly
      tempoAdjustment = 0.9999f;
    } else {
      // We're running slow, speed up slightly
      tempoAdjustment = 1.0001f;
    }
    
    // Apply tempo adjustment
    float newTempo = currentBpm * tempoAdjustment;
    if (newTempo >= MIN_BPM && newTempo <= MAX_BPM) {
      currentBpm = newTempo;
      uClock.setTempo(newTempo);
    }
  }
  
  // Instead of directly calling clockMe, let uClock's PLL handle sync
  if (abs(phaseError) > tickInterval / 2) {
    // Only force sync if we're significantly out of phase
    uClock.clockMe();
  }
}

void handleBeat(const SyncMessage &msg) {
  // Update BPM if changed
  if (msg.data.beat.bpm != currentBpm) {
    currentBpm = msg.data.beat.bpm;
    uClock.setTempo(currentBpm);
    Serial.print("Tempo updated: ");
    Serial.print(currentBpm);
    Serial.println(" BPM");
  }
  
  Serial.print("BEAT: position=");
  Serial.print(msg.data.beat.beatPosition);
  Serial.print(", multiplier=");
  Serial.println(msg.data.beat.multiplierIdx);
  
  // Update beat position and flash LEDs
  for (uint8_t i = 0; i < 2; i++) {
    if (channelEnabled[i]) {
      // Calculate beat position within pattern
      uint8_t oldBeat = currentBeats[i];
      currentBeats[i] = msg.data.beat.beatPosition % barLengths[i];
      
      Serial.print("Channel ");
      Serial.print(i);
      Serial.print(" beat: ");
      Serial.print(oldBeat);
      Serial.print(" -> ");
      Serial.println(currentBeats[i]);
    }
  }
  
  // This is a beat boundary - flash the LEDs
  lastBeatTime = millis();
  updateLEDs();
}

void handleBar(const SyncMessage &msg) {
  // Synchronize pattern position with master
  uint32_t globalBar = msg.data.bar.globalBar;
  
  Serial.print("BAR: global=");
  Serial.print(globalBar);
  Serial.print(", channels=");
  Serial.print(msg.data.bar.channelCount);
  Serial.print(", pattern=");
  Serial.print(msg.data.bar.activePattern);
  Serial.print(", length=");
  Serial.println(msg.data.bar.patternLength);
  
  // Reset beat positions on bar boundaries if needed
  for (uint8_t i = 0; i < 2; i++) {
    if (channelEnabled[i]) {
      uint8_t oldBeat = currentBeats[i];
      currentBeats[i] = 0;
      Serial.print("Channel ");
      Serial.print(i);
      Serial.print(" reset: ");
      Serial.print(oldBeat);
      Serial.println(" -> 0");
    }
  }
}

void handlePattern(const SyncMessage &msg) {
  uint8_t channelId = msg.data.pattern.channelId;
  
  if (channelId < 2) {
    patterns[channelId] = msg.data.pattern.pattern;
    barLengths[channelId] = msg.data.pattern.barLength;
    channelEnabled[channelId] = msg.data.pattern.enabled;
    currentBeats[channelId] = msg.data.pattern.currentBeat;
    
    Serial.print("PATTERN: channel=");
    Serial.print(channelId);
    Serial.print(", length=");
    Serial.print(barLengths[channelId]);
    Serial.print(", pattern=0x");
    Serial.print(patterns[channelId], HEX);
    Serial.print(", enabled=");
    Serial.print(channelEnabled[channelId]);
    Serial.print(", beat=");
    Serial.println(currentBeats[channelId]);
  }
}

void handleControl(const SyncMessage &msg) {
  Serial.print("CONTROL: command=");
  Serial.print(msg.data.control.command);
  Serial.print(", params=[");
  Serial.print(msg.data.control.param1);
  Serial.print(",");
  Serial.print(msg.data.control.param2);
  Serial.print(",");
  Serial.print(msg.data.control.param3);
  Serial.print("], value=");
  Serial.println(msg.data.control.value);

  switch (msg.data.control.command) {
    case CMD_START:
      isRunning = true;
      connectionLost = false;  // Reset connection state on start
      lastClockTime = millis(); // Reset clock timeout counter
      uClock.setMode(uClock.EXTERNAL_CLOCK);
      uClock.start();
      Serial.println("START: Transport started");
      break;
    case CMD_STOP:
      isRunning = false;
      uClock.stop();
      Serial.println("STOP: Transport stopped");
      break;
    case CMD_PAUSE:
      isRunning = false;
      uClock.stop();
      Serial.println("PAUSE: Transport paused");
      break;
    case CMD_RESET:
      // Reset all beat positions
      for (uint8_t i = 0; i < 2; i++) {
        uint8_t oldBeat = currentBeats[i];
        currentBeats[i] = 0;
        Serial.print("Channel ");
        Serial.print(i);
        Serial.print(" reset: ");
        Serial.print(oldBeat);
        Serial.println(" -> 0");
      }
      Serial.println("RESET: All channels reset");
      break;
    default:
      Serial.println("Unknown control command");
      break;
  }
}

bool isBeatActive(uint8_t channel, uint8_t beat) {
  if (beat >= barLengths[channel]) return false;
  return (patterns[channel] & (1 << beat)) != 0;
}

void onClockPulse(uint32_t tick) {
  // Only process on quarter note boundaries (96 PPQN)
  if (tick % 96 == 0) {
    uint32_t quarterNoteTick = tick / 96;
    
    // Update beat positions if we didn't get a BEAT message
    // This is fallback logic in case we miss messages
    for (uint8_t i = 0; i < 2; i++) {
      if (channelEnabled[i] && isRunning) {
        currentBeats[i] = (quarterNoteTick % barLengths[i]);
      }
    }
  }
}

void updateLEDs() {
  // Debug LED state
  Serial.println("\nLED Update:");
  Serial.print("Running: ");
  Serial.print(isRunning);
  Serial.print(", Connection lost: ");
  Serial.println(connectionLost);
  
  // For each channel
  for (uint8_t channel = 0; channel < 2; channel++) {
    if (channelEnabled[channel]) {
      float progress = (float)currentBeats[channel] / (float)barLengths[channel];
      Serial.print("Channel ");
      Serial.print(channel);
      Serial.print(": beat=");
      Serial.print(currentBeats[channel]);
      Serial.print("/");
      Serial.print(barLengths[channel]);
      Serial.print(" (");
      Serial.print(progress * 100);
      Serial.println("%)");
    }
  }
  
  // Clear all LEDs
  FastLED.clear();
  
  // If we're not running, just show a dim status light
  if (!isRunning) {
    leds[0] = CRGB::Green;
    leds[0].fadeToBlackBy(200); // Dim green to indicate standby
    FastLED.show();
    return;
  }
  
  // For each channel
  for (uint8_t channel = 0; channel < 2; channel++) {
    if (channelEnabled[channel]) {
      // Calculate progress for this channel
      float progress = (float)currentBeats[channel] / (float)barLengths[channel];
      int numLedsToLight = (int)(progress * (NUM_LEDS / 2));
      
      // Calculate LED range for this channel
      int startLed = channel * (NUM_LEDS / 2);
      int endLed = startLed + (NUM_LEDS / 2) - 1;
      
      // Light up LEDs to show progress
      for (int i = startLed; i <= endLed; i++) {
        if (i - startLed < numLedsToLight) {
          leds[i] = channelColors[channel];
        } else {
          // Show dim color for remaining LEDs
          leds[i] = channelColors[channel];
          leds[i].fadeToBlackBy(240);
        }
      }
    }
  }
  
  FastLED.show();
}

void checkConnection() {
  // Only check connection if we're running
  if (isRunning) {
    // Check if we haven't received a CLOCK message for a while
    if (!connectionLost && millis() - lastClockTime > CLOCK_TIMEOUT) {
      connectionLost = true;
      Serial.println("Connection to leader lost");
    }
  } else {
    // If we're not running, don't consider it a connection loss
    connectionLost = false;
  }
  
  // If connection is lost while running, blink the first LED red
  if (connectionLost && isRunning) {
    leds[0] = ((millis() / 500) % 2) ? CRGB::Red : CRGB::Black;
    FastLED.show();
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize LED strip
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(50);
  FastLED.clear();
  FastLED.show();
  
  // Set device as WiFi station
  WiFi.mode(WIFI_STA);
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register callback
  esp_now_register_recv_cb(onDataReceived);
  
  Serial.println("ESP-NOW Receiver initialized");
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Initialize uClock in external clock mode
  uClock.init();
  uClock.setMode(uClock.EXTERNAL_CLOCK);
  uClock.setOnPPQN(onClockPulse);
  uClock.setPPQN(uClock.PPQN_96);
  uClock.setTempo(currentBpm);
  
  // Show startup pattern
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::Green;
    FastLED.show();
    delay(20);
    leds[i] = CRGB::Black;
  }
  
  // Show ready indicator
  leds[0] = CRGB::Green;
  FastLED.show();
}

void loop() {
  static uint32_t lastDebugTime = 0;
  uint32_t now = millis();
  
  // Check for connection loss
  checkConnection();
  
  // Only update LEDs if we have a connection
  if (!connectionLost) {
    updateLEDs();
  }
  
  // Debug output every second
  if (now - lastDebugTime >= 1000) {
    Serial.print("Status - Running: ");
    Serial.print(isRunning);
    Serial.print(", Connected: ");
    Serial.print(!connectionLost);
    Serial.print(", Last clock: ");
    Serial.print(now - lastClockTime);
    Serial.println("ms ago");
    lastDebugTime = now;
  }
  
  // Small delay to prevent hogging the CPU
  delay(1);
} 