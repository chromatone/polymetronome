#ifndef BUZZER_CONTROLLER_H
#define BUZZER_CONTROLLER_H

#include <Arduino.h>
#include "MetronomeState.h"

// Sound configuration
struct SoundConfig
{
  static const uint32_t PWM_FREQ = 100000; // Higher PWM frequency for cleaner clicks
  static const uint8_t PWM_RES = 8;        // PWM resolution

  // Channel 1 - Lower pitched, sharp clicks
  static const uint16_t CH1_STRONG_FREQ = 2000; // Higher frequency for sharper attack
  static const uint16_t CH1_WEAK_FREQ = 1500;
  static const uint16_t CH1_STRONG_DUR = 15; // Shorter duration for click sound
  static const uint16_t CH1_WEAK_DUR = 10;
  static const uint8_t CH1_STRONG_VOL = 255; // Full volume for strong beats
  static const uint8_t CH1_WEAK_VOL = 128;

  // Channel 2 - Higher pitched, distinct clicks
  static const uint16_t CH2_STRONG_FREQ = 3000;
  static const uint16_t CH2_WEAK_FREQ = 2500;
  static const uint16_t CH2_STRONG_DUR = 12;
  static const uint16_t CH2_WEAK_DUR = 8;
  static const uint8_t CH2_STRONG_VOL = 255;
  static const uint8_t CH2_WEAK_VOL = 128;
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

  // Sound configurations using the new parameters
  const SoundParams ch1Strong = {SoundConfig::CH1_STRONG_FREQ, SoundConfig::CH1_STRONG_DUR, SoundConfig::CH1_STRONG_VOL};
  const SoundParams ch1Weak = {SoundConfig::CH1_WEAK_FREQ, SoundConfig::CH1_WEAK_DUR, SoundConfig::CH1_WEAK_VOL};
  const SoundParams ch2Strong = {SoundConfig::CH2_STRONG_FREQ, SoundConfig::CH2_STRONG_DUR, SoundConfig::CH2_STRONG_VOL};
  const SoundParams ch2Weak = {SoundConfig::CH2_WEAK_FREQ, SoundConfig::CH2_WEAK_DUR, SoundConfig::CH2_WEAK_VOL};

  void playSound(uint8_t channel, const SoundParams &params);
  void stopSound(uint8_t channel);

public:
  BuzzerController(uint8_t pin1, uint8_t pin2)
      : buzzerPin1(pin1), buzzerPin2(pin2) {}

  void init();
  void processBeat(uint8_t channel, BeatState beatState);
  void update();
};

#endif
