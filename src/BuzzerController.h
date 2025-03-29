#ifndef BUZZER_CONTROLLER_H
#define BUZZER_CONTROLLER_H

#include <Arduino.h>
#include "MetronomeState.h"

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

  // Sound configurations for each channel
  const SoundParams ch1Strong = {1000, 50, 128};
  const SoundParams ch1Weak = {750, 30, 64};
  const SoundParams ch2Strong = {1250, 45, 128};
  const SoundParams ch2Weak = {900, 25, 64};

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
