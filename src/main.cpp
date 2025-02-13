#include <Arduino.h>
#include "CommandSerial.h"
#include "MainCommand.h"

// Pin definitions
#define SOLENOID1_FET_PIN 16
#define SOLENOID1_PIEZO_PIN 17

// Timing constants
#define NORMAL_DURATION_MS 5
#define ACCENT_DURATION_MS 7
#define MIN_TRIGGER_INTERVAL_MS 100
#define MIN_BPM 20
#define MAX_BPM 500
#define DEFAULT_BPM 100
#define MAX_BEATS 16

// BLE constants
#define BLE_NAME "Metronome"

// Musical state
typedef struct
{
  uint32_t bpm;
  uint8_t beatsPerMeasure; // 1-16
  uint8_t subdivision;     // 2,4,8 (half, quarter, eighth notes)
  uint8_t currentPattern;  // Pattern number (1-based)
  bool enabled;
  uint32_t lastBeatTime;
  uint8_t currentBeat; // Current beat in measure (0-based)
} TimingState;

// Hardware state
typedef struct
{
  const uint8_t fetPin;
  const uint8_t piezoPin;
  const uint8_t normalDuration;
  const uint8_t accentDuration;
  uint32_t lastHallTime;
  bool lastState;
} SolenoidState;

// Global state
TimingState timing = {
    .bpm = DEFAULT_BPM,
    .beatsPerMeasure = 4,
    .subdivision = 4,
    .currentPattern = 1,
    .enabled = true,
    .lastBeatTime = 0,
    .currentBeat = 0};

SolenoidState solenoid = {
    .fetPin = SOLENOID1_FET_PIN,
    .piezoPin = SOLENOID1_PIEZO_PIN,
    .normalDuration = NORMAL_DURATION_MS,
    .accentDuration = ACCENT_DURATION_MS,
    .lastHallTime = 0,
    .lastState = false};

CommandSystem _commandSystem;
MainCommand _cmdMain;

// Get maximum pattern count for given beats (2^(beats-1) since first beat is always accented)
uint16_t getMaxPatterns(uint8_t beats)
{
  if (beats <= 1)
    return 1;
  return 1 << (beats - 1); // 2^(beats-1)
}

// Generate pattern from pattern number (1-based)
// Always sets first beat as accented (1)
uint16_t generatePattern(uint8_t beats, uint16_t patternNum)
{
  if (patternNum < 1)
    patternNum = 1;
  if (patternNum > getMaxPatterns(beats))
    patternNum = 1;

  // Convert to 0-based pattern number and shift left to make room for forced first beat
  uint16_t pattern = (patternNum - 1) << 1;
  // Set first beat as accented
  return pattern | 0x0001;
}

// Check if given beat should be accented according to current pattern
bool isAccentBeat(uint8_t currentBeat, uint16_t pattern)
{
  return (pattern & (1 << currentBeat)) != 0;
}

// Print all available patterns for current measure length
void printPatterns(uint8_t beats)
{
  uint16_t maxPatterns = getMaxPatterns(beats);
  Serial.printf("Available patterns for %d beats (1-%d):\n", beats, maxPatterns);

  for (uint16_t i = 1; i <= maxPatterns; i++)
  {
    uint16_t pattern = generatePattern(beats, i);
    Serial.printf("%2d: ", i);
    for (uint8_t b = 0; b < beats; b++)
    {
      Serial.print(isAccentBeat(b, pattern) ? 'X' : 'x');
    }
    Serial.println();
  }
}

// Calculate actual BPM based on subdivision
uint32_t getEffectiveBpm(uint32_t bpm, uint8_t subdivision)
{
  return (bpm * subdivision) / 4;
}

// Get milliseconds per beat considering subdivision
uint32_t getMsPerBeat(uint32_t bpm, uint8_t subdivision)
{
  return 60000 / getEffectiveBpm(bpm, subdivision);
}

void processMetronomeTick(TimingState *timing, SolenoidState *solenoid)
{
  uint32_t now = millis();
  uint32_t msPerBeat = getMsPerBeat(timing->bpm, timing->subdivision);
  uint32_t beatTime = now % msPerBeat;

  // Check if we've moved to a new beat
  if (now - timing->lastBeatTime >= msPerBeat)
  {
    timing->currentBeat = (timing->currentBeat + 1) % timing->beatsPerMeasure;
    timing->lastBeatTime = now - (now % msPerBeat);
  }

  uint16_t pattern = generatePattern(timing->beatsPerMeasure, timing->currentPattern);
  bool shouldAccent = isAccentBeat(timing->currentBeat, pattern);
  uint8_t duration = shouldAccent ? solenoid->accentDuration : solenoid->normalDuration;
  bool newState = beatTime < duration;

  if (newState != solenoid->lastState)
  {
    digitalWrite(solenoid->fetPin, newState);
    solenoid->lastState = newState;
  }
}

void IRAM_ATTR hallSensorISR()
{
  uint32_t now = millis();
  if (now - solenoid.lastHallTime > MIN_TRIGGER_INTERVAL_MS)
  {
    solenoid.lastHallTime = now;
  }
}

void setupCommands(MainCommand *cmd)
{
  cmd->addCallback("bpm", "Set bpm (20-300)", [](void *arg)
                   {
        std::vector<String> cmd = *(std::vector<String>*)arg;
        if (cmd.size() < 2) return;
        uint32_t newBpm = cmd[1].toInt();
        if (newBpm >= MIN_BPM && newBpm <= MAX_BPM) {
            timing.bpm = newBpm;
            Serial.printf("BPM: %d (effective: %d)\n", 
                newBpm, 
                getEffectiveBpm(newBpm, timing.subdivision));
        } });

  cmd->addCallback("measure", "Set beats per measure (1-16)", [](void *arg)
                   {
        std::vector<String> cmd = *(std::vector<String>*)arg;
        if (cmd.size() < 2) return;
        uint8_t beats = cmd[1].toInt();
        if (beats >= 1 && beats <= MAX_BEATS) {
            timing.beatsPerMeasure = beats;
            timing.currentPattern = 1;  // Reset to first pattern
            timing.currentBeat = 0;     // Reset beat counter
            printPatterns(beats);
        } });

  cmd->addCallback("pattern", "Set beat pattern", [](void *arg)
                   {
        std::vector<String> cmd = *(std::vector<String>*)arg;
        if (cmd.size() < 2) return;
        uint16_t pattern = cmd[1].toInt();
        uint16_t maxPatterns = getMaxPatterns(timing.beatsPerMeasure);
        if (pattern >= 1 && pattern <= maxPatterns) {
            timing.currentPattern = pattern;
            Serial.printf("Pattern set to %d: ", pattern);
            uint16_t pat = generatePattern(timing.beatsPerMeasure, pattern);
            for (uint8_t b = 0; b < timing.beatsPerMeasure; b++) {
                Serial.print(isAccentBeat(b, pat) ? 'X' : 'x');
            }
            Serial.println();
        } });

  cmd->addCallback("subdivision", "Set subdivision (2=half,4=quarter,8=eighth)", [](void *arg)
                   {
        std::vector<String> cmd = *(std::vector<String>*)arg;
        if (cmd.size() < 2) return;
        uint8_t sub = cmd[1].toInt();
        if (sub == 2 || sub == 4 || sub == 8) {
            timing.subdivision = sub;
            Serial.printf("Subdivision: 1/%d (effective BPM: %d)\n", 
                sub, 
                getEffectiveBpm(timing.bpm, sub));
        } });
}

/* -------------------------------------------------------------------------- */
/*                                     BLE                                    */
/* -------------------------------------------------------------------------- */
#include "ble_utils/BLEMsgStructure.h"
#include "ble_utils/BLEMetronomeServer.h"
void onBleConnected();
void onBleDisconnect();
void onControlChange(uint8_t channel, uint8_t controller, uint8_t value, uint16_t timestamp);

void BLE_init()
{
  BLEMetronomeServer.begin(BLE_NAME);
	BLEMetronomeServer.setOnConnectCallback(onBleConnected);
	BLEMetronomeServer.setOnDisconnectCallback(onBleDisconnect); 
	BLEMetronomeServer.setControlChangeCallback(onControlChange);

}

void onBleConnected()
{
  Serial.println("BLE Connected");
}

void onBleDisconnect()
{
  Serial.println("BLE Disconnected");
}

void onControlChange(uint8_t channel, uint8_t controller, uint8_t value, uint16_t timestamp)
{
  int channel_actual = channel + 1;
  Serial.printf("Control Change, channel %d, controller %d, value %d\n", channel_actual, controller, value); 
  // Control BPM by CH15 CC2
  if (channel_actual == 15 && controller == 2) {
    timing.bpm = map(value, 0, 127, MIN_BPM, MAX_BPM);
    Serial.printf("BPM: %d (effective: %d)\n", 
        timing.bpm, 
        getEffectiveBpm(timing.bpm, timing.subdivision));}
  
}



void setup()
{
  Serial.begin(115200);
  BLE_init();
  pinMode(solenoid.fetPin, OUTPUT);
  pinMode(solenoid.piezoPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(solenoid.piezoPin), hallSensorISR, FALLING);

  setupCommands(&_cmdMain);
  _commandSystem.registerClass(&_cmdMain);

  // Print initial patterns
  printPatterns(timing.beatsPerMeasure);
}

void loop()
{
  _commandSystem.parser();
  if (timing.enabled)
  {
    processMetronomeTick(&timing, &solenoid);
  }
}