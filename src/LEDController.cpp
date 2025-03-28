#include "LEDController.h"

#define LED_PIN 27 // Using GPIO27 for LED strip
#define NUM_LEDS 33
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

LEDController::LEDController()
    : leds(nullptr),
      numLeds(NUM_LEDS)
{
  // Initialize flash states
  globalFlash = {0, false};
  for (int i = 0; i < FIXED_CHANNEL_COUNT; i++)
  {
    channelFlash[i] = {0, false};
  }

  // Clear all arrays
  memset(sectionStarts, 0, sizeof(sectionStarts));
  memset(sectionSizes, 0, sizeof(sectionSizes));

  // Calculate available space for patterns after reserving space for
  // center LED and blinkers
  const uint8_t centerPos = NUM_LEDS / 2;
  const uint8_t patternSpace = (NUM_LEDS - CENTER_LED - (2 * BLINKER_SIZE)) / 2;

  // Set section sizes
  sectionSizes[CH1_PATTERN] = patternSpace;
  sectionSizes[CH2_PATTERN] = patternSpace;
  sectionSizes[GLOBAL_BPM] = CENTER_LED;
  sectionSizes[CH1_BLINK] = BLINKER_SIZE;
  sectionSizes[CH2_BLINK] = BLINKER_SIZE;

  // Calculate positions from left to right
  uint8_t currentPos = 0;

  // Channel 1 pattern (starts from left)
  sectionStarts[CH1_PATTERN] = currentPos;
  currentPos += sectionSizes[CH1_PATTERN];

  // Channel 1 blink
  sectionStarts[CH1_BLINK] = currentPos;
  currentPos += sectionSizes[CH1_BLINK];

  // Global BPM (center)
  sectionStarts[GLOBAL_BPM] = centerPos;
  currentPos = centerPos + CENTER_LED;

  // Channel 2 blink
  sectionStarts[CH2_BLINK] = currentPos;
  currentPos += sectionSizes[CH2_BLINK];

  // Channel 2 pattern
  sectionStarts[CH2_PATTERN] = currentPos;

  // Debug output
  Serial.println("\nLED Strip Layout:");
  const char *sectionNames[] = {
      "CH1_PATTERN", "CH1_BLINK", "GLOBAL_BPM", "CH2_BLINK", "CH2_PATTERN"};

  for (int i = 0; i < SECTION_COUNT; i++)
  {
    uint8_t end = sectionStarts[i] + sectionSizes[i] - 1;
    Serial.printf("%s: start=%d, size=%d (end=%d)\n",
                  sectionNames[i],
                  sectionStarts[i],
                  sectionSizes[i],
                  end);

    // Validate bounds
    if (sectionStarts[i] + sectionSizes[i] > NUM_LEDS)
    {
      Serial.printf("ERROR: Section %s exceeds LED strip bounds!\n",
                    sectionNames[i]);
    }
  }

  Serial.printf("Total LEDs: %d, Center: %d\n", NUM_LEDS, centerPos);
}

LEDController::~LEDController()
{
  if (leds)
  {
    delete[] leds;
  }
}

bool LEDController::isFlashActive(const FlashState &flash) const
{
  if (!flash.isFlashing)
    return false;
  return (millis() - flash.startTime) < LED_BEAT_DURATION_MS;
}

void LEDController::startFlash(FlashState &flash)
{
  flash.startTime = millis();
  flash.isFlashing = true;
}

void LEDController::init()
{
  leds = new CRGB[NUM_LEDS];
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS)
      .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(50);

  // Ensure complete initialization
  clear();
  FastLED.show();
  delay(100); // Allow strip to stabilize

  startupAnimation();
}

CRGB LEDController::applyBrightness(CRGB color, uint8_t brightness) const
{
  return color.nscale8(brightness);
}

void LEDController::drawPattern(const MetronomeChannel &channel, uint8_t startLed,
                                uint8_t size, bool reverse, const MetronomeState &state,
                                const CRGB &baseColor)
{
  if (!channel.isEnabled())
  {
    fill_solid(leds + startLed, size, CRGB::Black);
    return;
  }

  uint8_t barLength = channel.getBarLength();
  uint8_t patternSpace = calculatePatternSpace(barLength);

  // Calculate pattern start position to align with center
  uint8_t patternStartOffset = reverse ? (size - patternSpace) : 0;

  // First fill all LEDs with black
  fill_solid(leds + startLed, size, CRGB::Black);

  // For channel 2 in polyrhythm mode, calculate current beat differently
  bool isChannel2Polyrhythm = (channel.getId() == 1 && state.isPolyrhythm());
  uint8_t currentBeat;

  if (isChannel2Polyrhythm)
  {
    // Calculate the exact fractional position for channel 2
    uint8_t ch1Length = state.getChannel(0).getBarLength();
    float cycleProgress = float(state.globalTick % ch1Length) / float(ch1Length);
    cycleProgress += state.tickFraction / float(ch1Length);
    float beatPosition = cycleProgress * float(barLength);
    currentBeat = uint8_t(beatPosition) % barLength;
  }
  else
  {
    currentBeat = channel.getCurrentBeat();
  }

  // Draw pattern up to the current bar length
  for (uint8_t i = 0; i < patternSpace; i++)
  {
    uint8_t mappedPos = patternStartOffset + (reverse ? (patternSpace - 1 - i) : i);
    if (mappedPos < size)
    {
      bool isActive = channel.getPatternBit(i);
      bool isCurrent = (i == currentBeat);

      CRGB color;
      if (isCurrent)
      {
        color = applyBrightness(baseColor, isActive ? ACTIVE_BRIGHTNESS : NORMAL_BRIGHTNESS);
      }
      else if (isActive)
      {
        color = applyBrightness(baseColor, INACTIVE_BRIGHTNESS);
      }
      else
      {
        color = applyBrightness(baseColor, INACTIVE_BRIGHTNESS / 4);
      }

      leds[startLed + mappedPos] = color;
    }
  }
}

uint8_t LEDController::calculatePatternSpace(uint8_t barLength) const
{
  // Return exact number of LEDs needed for the pattern
  return min(barLength, MAX_PATTERN_SIZE);
}

uint8_t LEDController::mapPatternPosition(uint8_t position, uint8_t size, bool reverse) const
{
  if (reverse)
  {
    return size - 1 - position;
  }
  return position;
}

void LEDController::updateSection(StripSection section, const MetronomeState &state)
{
  // Validate section bounds
  if (section >= SECTION_COUNT)
    return;

  uint8_t start = sectionStarts[section];
  uint8_t size = sectionSizes[section];

  // Validate LED array bounds
  if (start + size > NUM_LEDS)
  {
    Serial.printf("LED bounds error: section=%d, start=%d, size=%d\n",
                  section, start, size);
    return;
  }

  switch (section)
  {
  case GLOBAL_BPM:
    // Center LED for global tempo
    if (state.isRunning && !state.isPaused && isFlashActive(globalFlash))
    {
      leds[start] = CRGB::White;
    }
    else
    {
      leds[start] = CRGB::Black;
    }
    break;

  case CH1_PATTERN:
  case CH2_PATTERN:
  {
    uint8_t ch = (section == CH1_PATTERN) ? 0 : 1;
    const MetronomeChannel &channel = state.getChannel(ch);
    const CRGB &channelColor = (ch == 0) ? CH1_COLOR : CH2_COLOR;

    drawPattern(channel, start, size, (section == CH1_PATTERN), state, channelColor);
    break;
  }

  case CH1_BLINK:
  case CH2_BLINK:
  {
    uint8_t ch = (section == CH1_BLINK) ? 0 : 1;
    const MetronomeChannel &channel = state.getChannel(ch);
    bool isFlashing = state.isRunning && !state.isPaused &&
                      channel.isEnabled() && isFlashActive(channelFlash[ch]);

    uint8_t patternLength = calculatePatternSpace(channel.getBarLength());
    uint8_t blinkerPos;

    if (section == CH1_BLINK)
    {
      // Always place CH1 blinker at the start of its section
      blinkerPos = sectionStarts[CH1_BLINK];
    }
    else
    {
      // Always place CH2 blinker at the start of its section
      blinkerPos = sectionStarts[CH2_BLINK];
    }

    // Safety check
    if (blinkerPos < NUM_LEDS)
    {
      const CRGB &channelColor = (ch == 0) ? CH1_COLOR : CH2_COLOR;
      leds[blinkerPos] = isFlashing ? channelColor : CRGB::Black;
    }
    break;
  }
  }
}

void LEDController::update(const MetronomeState &state)
{
  // Update all sections in defined order
  for (int i = 0; i < SECTION_COUNT; i++)
  {
    updateSection(static_cast<StripSection>(i), state);
  }
  FastLED.show();
}

void LEDController::onGlobalBeat()
{
  startFlash(globalFlash);
}

void LEDController::onChannelBeat(uint8_t channel)
{
  if (channel < FIXED_CHANNEL_COUNT)
  {
    startFlash(channelFlash[channel]);
  }
}

CRGB LEDController::getPatternColor(const MetronomeChannel &channel, uint8_t position, const MetronomeState &state) const
{
  if (!channel.isEnabled())
    return CRGB::Black;

  bool isCurrentBeat;
  uint8_t currentPosition;

  if (channel.getId() == 1 && state.isPolyrhythm())
  {
    uint8_t ch1Length = state.getChannel(0).getBarLength();
    uint8_t ch2Length = channel.getBarLength();

    if (ch1Length > 0 && ch2Length > 0)
    {
      // Calculate the normalized progress (0.0 to 1.0) within channel 1's cycle
      float cycleProgress = float(state.globalTick % ch1Length) / float(ch1Length);

      // Add fractional part for smooth movement
      cycleProgress += state.tickFraction / float(ch1Length);

      // Calculate channel 2's beat position
      float beatPosition = cycleProgress * float(ch2Length);
      currentPosition = uint8_t(beatPosition) % ch2Length;

      // Calculate how far we are into the current beat (0.0 to 1.0)
      float beatFraction = beatPosition - float(currentPosition);

      // Use a wider window for the "current" position to match the LED flash duration
      // LED_BEAT_DURATION_MS is typically 100ms, so we'll use a proportional window
      float windowSize = float(LED_BEAT_DURATION_MS) / (60000.0f / state.getEffectiveBpm());

      // Position is current if we're within the window
      isCurrentBeat = (position == currentPosition) && (beatFraction < windowSize);
    }
    else
    {
      return CRGB::Black;
    }
  }
  else
  {
    // Normal pattern visualization for channel 1 and polymeter
    currentPosition = channel.getCurrentBeat();
    isCurrentBeat = (position == currentPosition);
  }

  // Return appropriate color based on pattern and current position
  if (isCurrentBeat && channel.getPatternBit(position))
  {
    return CRGB::White; // Active beat, current position
  }
  else if (isCurrentBeat)
  {
    return CRGB::Red; // Inactive beat, current position
  }
  else if (channel.getPatternBit(position))
  {
    return CRGB(32, 32, 32); // Active beat, not current
  }

  return CRGB::Black; // Inactive beat, not current
}

void LEDController::clear()
{
  FastLED.clear();
  FastLED.show();
}

void LEDController::setBrightness(uint8_t brightness)
{
  FastLED.setBrightness(brightness);
}

void LEDController::startupAnimation()
{
  // Show startup animation
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Green;
    FastLED.show();
    delay(20);
    leds[i] = CRGB::Black;
  }

  // Show ready state
  leds[0] = CRGB::Green;
  FastLED.show();
}
