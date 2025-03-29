#ifndef BUZZER_CONTROLLER_H
#define BUZZER_CONTROLLER_H

#include <Arduino.h>
#include "MetronomeState.h"
#include <Ticker.h>

// Sound configuration
struct SoundConfig
{
  static const uint32_t PWM_FREQ = 10000;
  static const uint8_t PWM_RES = 8;

  // Channel 1 - Note A (440 Hz)
  static const uint16_t CH1_FREQ = 440;      // A4
  static const uint16_t CH1_DUR = 50;        // Same duration for all beats
  static const uint8_t CH1_STRONG_VOL = 200; // Full volume for strong beat
  static const uint8_t CH1_WEAK_VOL = 128;   // Half volume for weak beats

  // Channel 2 - Note C (523 Hz)
  static const uint16_t CH2_FREQ = 523;      // C5
  static const uint16_t CH2_DUR = 50;        // Same duration for all beats
  static const uint8_t CH2_STRONG_VOL = 200; // Full volume for strong beat
  static const uint8_t CH2_WEAK_VOL = 128;   // Half volume for weak beats
};

class BuzzerController
{
private:
  uint8_t buzzerPin1;
  uint8_t buzzerPin2;

  struct SoundState
  {
    bool isPlaying = false;
    unsigned long startTime = 0;
    unsigned long soundDuration = 0;
  };

  SoundState channel1State;
  SoundState channel2State;

  struct SoundParams
  {
    uint16_t frequency;
    uint16_t duration;
    uint8_t volume;
  };

  // Simplified sound configurations
  const SoundParams ch1Strong = {SoundConfig::CH1_FREQ, SoundConfig::CH1_DUR, SoundConfig::CH1_STRONG_VOL};
  const SoundParams ch1Weak = {SoundConfig::CH1_FREQ, SoundConfig::CH1_DUR, SoundConfig::CH1_WEAK_VOL};
  const SoundParams ch2Strong = {SoundConfig::CH2_FREQ, SoundConfig::CH2_DUR, SoundConfig::CH2_STRONG_VOL};
  const SoundParams ch2Weak = {SoundConfig::CH2_FREQ, SoundConfig::CH2_DUR, SoundConfig::CH2_WEAK_VOL};

  void playSound(uint8_t channel, const SoundParams &params);
  void stopSound(uint8_t channel);

  Ticker soundTicker;
  static BuzzerController *_instance;
  static void IRAM_ATTR endSoundCallback();

  void endSound(uint8_t channel);
  bool isPlaying = false;

public:
  BuzzerController(uint8_t pin1, uint8_t pin2)
      : buzzerPin1(pin1), buzzerPin2(pin2)
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
