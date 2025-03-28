#pragma once

#include <FastLED.h>
#include "MetronomeState.h"
#include "config.h"

class LEDController
{
private:
  CRGB *leds;
  const uint8_t numLeds;
  const uint8_t mainSection;
  const uint8_t channelSection;
  static const uint8_t LEDS_PER_INDICATOR = 3;

  struct FlashState
  {
    uint32_t startTime;
    bool isFlashing;
  };

  FlashState globalFlash;
  FlashState channelFlash[FIXED_CHANNEL_COUNT];

  void updateGlobalIndicator(const MetronomeState &state);
  void updateChannelIndicators(const MetronomeState &state);
  void updateChannelPatterns(const MetronomeState &state);
  bool isFlashActive(const FlashState &flash) const;
  void startFlash(FlashState &flash);
  CRGB getPatternColor(const MetronomeChannel &channel, uint8_t position) const;

public:
  LEDController();
  ~LEDController();

  void init();
  void update(const MetronomeState &state);
  void clear();
  void setBrightness(uint8_t brightness);
  void startupAnimation();

  // Beat handlers
  void onGlobalBeat();
  void onChannelBeat(uint8_t channel);
};
