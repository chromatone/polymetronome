#pragma once

#include <FastLED.h>
#include "MetronomeState.h"
#include "config.h"

// Define strip sections and their order
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
  static const uint8_t LEDS_PER_INDICATOR = 3;
  static const uint8_t PATTERN_SECTION_SIZE = 11; // (33 - 5*3) / 2 = 11 LEDs per pattern

  // New constants for layout
  static const uint8_t CENTER_LED = 1;        // Space for central BPM indicator
  static const uint8_t MAX_PATTERN_SIZE = 16; // Maximum pattern length
  static const uint8_t BLINKER_SIZE = 1;      // Size of channel beat indicators

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

  // Channel colors - declare as static constexpr
  static constexpr CRGB CH1_COLOR = CRGB(0, 100, 255); // Blue
  static constexpr CRGB CH2_COLOR = CRGB(255, 100, 0); // Orange

  // Brightness levels
  static const uint8_t INACTIVE_BRIGHTNESS = 25; // 10%
  static const uint8_t NORMAL_BRIGHTNESS = 128;  // 50%
  static const uint8_t ACTIVE_BRIGHTNESS = 255;  // 100%

  // Helper method to apply brightness to a color
  CRGB applyBrightness(CRGB color, uint8_t brightness) const;

  // Helper methods
  void updateSection(StripSection section, const MetronomeState &state);
  bool isFlashActive(const FlashState &flash) const;
  void startFlash(FlashState &flash);
  CRGB getPatternColor(const MetronomeChannel &channel, uint8_t position, const MetronomeState &state) const;
  float getPolyrhythmProgress(const MetronomeChannel &channel) const;

  // Helper methods for bidirectional pattern display
  void drawPattern(const MetronomeChannel &channel, uint8_t startLed,
                   uint8_t size, const MetronomeState &state, const CRGB &baseColor);
  uint8_t calculatePatternSpace(uint8_t barLength) const;

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
