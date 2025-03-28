#pragma once

#include <FastLED.h>
#include "MetronomeState.h"
#include "config.h"

// Define strip sections and their order
enum StripSection
{
  CH1_BLINK = 0,
  CH1_PATTERN,
  GLOBAL_BPM,
  CH2_PATTERN,
  CH2_BLINK,
  SECTION_COUNT
};

class LEDController
{
private:
  CRGB *leds;
  const uint8_t numLeds;
  static const uint8_t LEDS_PER_INDICATOR = 3;
  static const uint8_t PATTERN_SECTION_SIZE = 11; // (33 - 5*3) / 2 = 11 LEDs per pattern

  // Section start positions (calculated in constructor)
  uint8_t sectionStarts[SECTION_COUNT];
  uint8_t sectionSizes[SECTION_COUNT];

  struct FlashState
  {
    uint32_t startTime;
    bool isFlashing;
  };

  FlashState globalFlash;
  FlashState channelFlash[FIXED_CHANNEL_COUNT];

  // Helper methods
  void updateSection(StripSection section, const MetronomeState &state);
  bool isFlashActive(const FlashState &flash) const;
  void startFlash(FlashState &flash);
  CRGB getPatternColor(const MetronomeChannel &channel, uint8_t position, const MetronomeState &state) const;
  float getPolyrhythmProgress(const MetronomeChannel &channel) const;

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
