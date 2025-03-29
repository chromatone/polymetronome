#ifndef BUZZER_CONTROLLER_H
#define BUZZER_CONTROLLER_H

#include <Arduino.h>
#include "MetronomeState.h"

class BuzzerController
{
private:
  uint8_t buzzerPin;
  unsigned long startTime;
  unsigned long soundDuration;
  bool isPlaying = false;

  // Sound parameters for each channel and beat type
  struct SoundParams
  {
    uint16_t frequency; // Base frequency in Hz
    uint16_t duration;  // Duration in milliseconds
    uint8_t volume;     // PWM duty cycle (0-255)
  };

  // Sound configurations for each channel and beat type
  const SoundParams ch1Strong = {2000, 50, 128}; // Higher pitch, longer duration
  const SoundParams ch1Weak = {1500, 30, 64};    // Lower volume and shorter
  const SoundParams ch2Strong = {2500, 45, 128}; // Different pitch for distinction
  const SoundParams ch2Weak = {2000, 25, 64};    // Different pitch but softer

  void playSound(const SoundParams &params);
  void stopSound();

public:
  BuzzerController(uint8_t pin) : buzzerPin(pin) {}

  void init();
  void processBeat(uint8_t channel, BeatState beatState);
  void update();
};

#endif // BUZZER_CONTROLLER_H
