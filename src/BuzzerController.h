#ifndef BUZZER_CONTROLLER_H
#define BUZZER_CONTROLLER_H

#include <Arduino.h>
#include "MetronomeState.h"
#include <Ticker.h>

// Sound configuration
struct SoundConfig
{
  static const uint32_t PWM_FREQ = 5000; // Reduced from 10000
  static const uint8_t PWM_RES = 10;     // Increased from 8

  // Channel 1 - Note A
  static const uint16_t CH1_FREQ = 440;      // A4
  static const uint16_t CH1_DUR = 20;        // Same duration for all beats
  static const uint8_t CH1_STRONG_VOL = 200; // Full volume for strong beat
  static const uint8_t CH1_WEAK_VOL = 128;   // Half volume for weak beats

  // Channel 2 - Note E
  static const uint16_t CH2_FREQ = 659.26;   // E4
  static const uint16_t CH2_DUR = 20;        // Same duration for all beats
  static const uint8_t CH2_STRONG_VOL = 100; // Full volume for strong beat
  static const uint8_t CH2_WEAK_VOL = 58;    // Half volume for weak beats

  static const uint8_t RAMP_STEPS = 10; // Steps for volume ramping
  static const uint8_t RAMP_DELAY = 1;  // Milliseconds between ramp steps

  // Sweep configuration
  static const uint16_t SWEEP_MULTIPLIER = 3; // Initial frequency multiplier
  static const uint8_t SWEEP_DURATION = 8;    // Duration of sweep in ms
  static const uint8_t SWEEP_STEPS = 8;       // Number of frequency steps

  // PWM Pattern configuration
  static const uint8_t PATTERN_LENGTH = 8;
  static const uint8_t PATTERN_REPEAT = 4; // Number of pattern repeats during attack

  // Noise configuration
  static const uint8_t NOISE_STRONG = 80;     // Noise mix ratio for strong beats
  static const uint8_t NOISE_WEAK = 40;       // Noise mix ratio for weak beats
  static const uint8_t NOISE_UPDATE_RATE = 1; // ms between noise updates

  // Default volumes moved to MetronomeChannel class
  static const uint8_t VOLUME_STEP = 16; // Step size for volume adjustment
};

class BuzzerController
{
private:
  // Only declare the patterns, don't define them here
  static const uint8_t PROGMEM strongPattern[SoundConfig::PATTERN_LENGTH];
  static const uint8_t PROGMEM weakPattern[SoundConfig::PATTERN_LENGTH];

  enum PinMode
  {
    MODE_INPUT,
    MODE_DIGITAL,
    MODE_PWM
  };

  uint8_t buzzerPin1;
  uint8_t buzzerPin2;
  PinMode currentMode = MODE_INPUT;
  bool pinsAttached = false;

  struct SoundState
  {
    bool isPlaying = false;
    unsigned long startTime = 0;
    unsigned long soundDuration = 0;
    uint8_t currentVolume = 0;
  };

  SoundState channel1State;
  SoundState channel2State;

  struct SoundParams
  {
    uint16_t frequency;
    uint16_t duration;
    uint8_t volume; // Now controlled by channel
    bool useFrequencySweep;
    const uint8_t *pwmPattern;
    uint8_t noiseRatio; // Added parameter
  };

  // Now declare sound configurations after patterns and params are defined
  const SoundParams ch1Strong;
  const SoundParams ch1Weak;
  const SoundParams ch2Strong;
  const SoundParams ch2Weak;

  void playSound(uint8_t channel, const SoundParams &params);
  void stopSound(uint8_t channel);

  void attachPins();
  void detachPins();
  void rampVolume(uint8_t channel, uint8_t targetVolume);

  void setPinMode(PinMode mode);
  void silencePins();

  void performFrequencySweep(uint8_t channel, uint16_t targetFreq);
  void applyPWMPattern(uint8_t channel, const uint8_t *pattern, uint8_t volume);
  void applyNoiseMix(uint8_t channel, uint8_t volume, uint8_t noiseRatio);

  // Helper method to get channel-adjusted volume
  uint8_t getAdjustedVolume(uint8_t channel, uint8_t baseVolume) const;

  Ticker soundTicker;
  static BuzzerController *_instance;
  static void IRAM_ATTR endSoundCallback();

  void endSound(uint8_t channel);
  bool isPlaying = false;

public:
  BuzzerController(uint8_t pin1, uint8_t pin2)
      : buzzerPin1(pin1), buzzerPin2(pin2),
        ch1Strong{SoundConfig::CH1_FREQ, SoundConfig::CH1_DUR,
                  SoundConfig::CH1_STRONG_VOL, true, strongPattern, SoundConfig::NOISE_STRONG},
        ch1Weak{SoundConfig::CH1_FREQ, SoundConfig::CH1_DUR,
                SoundConfig::CH1_WEAK_VOL, false, weakPattern, SoundConfig::NOISE_WEAK},
        ch2Strong{SoundConfig::CH2_FREQ, SoundConfig::CH2_DUR,
                  SoundConfig::CH2_STRONG_VOL, true, strongPattern, SoundConfig::NOISE_STRONG},
        ch2Weak{SoundConfig::CH2_FREQ, SoundConfig::CH2_DUR,
                SoundConfig::CH2_WEAK_VOL, false, weakPattern, SoundConfig::NOISE_WEAK}
  {
    _instance = this;
  }

  ~BuzzerController()
  {
    if (_instance == this)
    {
      _instance = nullptr;
    }
  }

  void init();
  void processBeat(uint8_t channel, BeatState beatState);
  void update();
};

#endif
