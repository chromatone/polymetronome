#pragma once

#include <FastLED.h>
#include "MetronomeState.h"
#include "config.h"

enum StripSection
{
  BPM_START,
  CH1_BLINK,
  CH1_PATTERN,
  BPM_MID,
  CH2_BLINK,
  CH2_PATTERN,
  BPM_END,
  SECTION_COUNT
};

class LEDController
{
private:
  CRGB *leds;
  const uint8_t numLeds;
  static const uint8_t MAX_PATTERN_SIZE = 16;
  static const uint8_t BLINKER_SIZE = 1;

  struct FlashState
  {
    uint32_t startTime;
    bool isFlashing;
  };

  FlashState globalFlash;
  FlashState channelFlash[FIXED_CHANNEL_COUNT];

  static constexpr CRGB CH1_COLOR = CRGB(0, 100, 255);
  static constexpr CRGB CH2_COLOR = CRGB(255, 100, 0);

  bool isFlashActive(const FlashState &flash) const;
  void startFlash(FlashState &flash);
  void drawPattern(const MetronomeChannel &channel, uint8_t startLed,
                   uint8_t size, const MetronomeState &state,
                   const CRGB &baseColor);
  uint8_t calculatePatternSpace(uint8_t barLength) const;
  void updateSection(StripSection section, const MetronomeState &state);

public:
  LEDController();
  ~LEDController();
  void init();
  void update(const MetronomeState &state);
  void clear();
  void setBrightness(uint8_t brightness);
  void startupAnimation();
  void onGlobalBeat();
  void onChannelBeat(uint8_t channel);
};
