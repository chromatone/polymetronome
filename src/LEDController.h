#pragma once

#include <FastLED.h>
#include "MetronomeState.h"
#include "config.h"

class LEDController
{
private:
  CRGB *leds;
  uint8_t numLeds;
  uint8_t mainSection;    // Number of LEDs in main tempo section
  uint8_t channelSection; // Number of LEDs per channel section

  uint32_t lastBeatTime = 0;
  uint32_t beatDuration = 100; // Flash duration in ms

  struct ChannelState
  {
    bool enabled;
    uint8_t currentBeat;
    uint8_t barLength;
    uint16_t pattern;
  };
  ChannelState channels[FIXED_CHANNEL_COUNT];

public:
  LEDController();
  ~LEDController();

  void init();
  void update(const MetronomeState &state);
  void onBeat(uint8_t channel, BeatState beatState);
  void clear();
  void setBrightness(uint8_t brightness);
  void startupAnimation();
};
