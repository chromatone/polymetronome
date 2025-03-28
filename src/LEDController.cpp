#include "LEDController.h"

#define LED_PIN 27 // Using GPIO27 for LED strip
#define NUM_LEDS 33
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

LEDController::LEDController()
    : leds(nullptr),
      numLeds(NUM_LEDS)
{
  // Initialize flash states and clear arrays
  globalFlash = {0, false};
  for (int i = 0; i < FIXED_CHANNEL_COUNT; i++)
  {
    channelFlash[i] = {0, false};
  }
  memset(sectionStarts, 0, sizeof(sectionStarts));
  memset(sectionSizes, 0, sizeof(sectionSizes));
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
                                uint8_t size, const MetronomeState &state,
                                const CRGB &baseColor)
{
  if (!channel.isEnabled())
  {
    fill_solid(leds + startLed, size, CRGB::Black);
    return;
  }

  // First fill all LEDs with black
  fill_solid(leds + startLed, size, CRGB::Black);

  // For channel 2 in polyrhythm mode, calculate current beat differently
  bool isChannel2Polyrhythm = (channel.getId() == 1 && state.isPolyrhythm());
  uint8_t currentBeat;

  if (isChannel2Polyrhythm)
  {
    uint8_t ch1Length = state.getChannel(0).getBarLength();
    float cycleProgress = float(state.globalTick % ch1Length) / float(ch1Length);
    cycleProgress += state.tickFraction / float(ch1Length);
    float beatPosition = cycleProgress * float(channel.getBarLength());
    currentBeat = uint8_t(beatPosition) % channel.getBarLength();
  }
  else
  {
    currentBeat = channel.getCurrentBeat();
  }

  // Draw pattern
  uint8_t barLength = channel.getBarLength();
  uint8_t patternSpace = calculatePatternSpace(barLength);

  for (uint8_t i = 0; i < patternSpace && i < size; i++)
  {
    bool isActive = channel.getPatternBit(i);
    bool isCurrent = (i == currentBeat);

    CRGB color;
    if (isCurrent && isActive)
    {
      color = CRGB::White;
    }
    else if (isActive)
    {
      color = baseColor;
    }
    else if (isCurrent)
    {
      color = CRGB(baseColor).nscale8(64);
    }
    else
    {
      color = CRGB(baseColor).nscale8(25);
    }

    leds[startLed + i] = color;
  }
}

uint8_t LEDController::calculatePatternSpace(uint8_t barLength) const
{
  // Return exact number of LEDs needed for the pattern
  return min(barLength, MAX_PATTERN_SIZE);
}

void LEDController::updateSection(StripSection section, const MetronomeState &state)
{
  // Dynamically calculate section positions based on CH1 pattern length
  uint8_t ch1Length = state.getChannel(0).getBarLength();
  uint8_t ch1PatternSpace = calculatePatternSpace(ch1Length);

  // Calculate starting positions dynamically
  uint8_t currentPos = 0;

  // BPM start
  if (section == BPM_START)
  {
    leds[currentPos] = (state.isRunning && !state.isPaused && isFlashActive(globalFlash))
                           ? CRGB::White
                           : CRGB::Black;
    return;
  }
  currentPos += 1;

  // CH1 blink
  if (section == CH1_BLINK)
  {
    if (state.isRunning && !state.isPaused &&
        state.getChannel(0).isEnabled() && isFlashActive(channelFlash[0]))
    {
      leds[currentPos] = CH1_COLOR;
    }
    else
    {
      leds[currentPos] = CRGB::Black;
    }
    return;
  }
  currentPos += BLINKER_SIZE;

  // CH1 pattern
  if (section == CH1_PATTERN)
  {
    drawPattern(state.getChannel(0), currentPos, ch1PatternSpace, state, CH1_COLOR);
    return;
  }
  currentPos += ch1PatternSpace;

  // BPM mid
  if (section == BPM_MID)
  {
    leds[currentPos] = (state.isRunning && !state.isPaused && isFlashActive(globalFlash))
                           ? CRGB::White
                           : CRGB::Black;
    return;
  }
  currentPos += 1;

  // CH2 blink
  if (section == CH2_BLINK)
  {
    if (state.isRunning && !state.isPaused &&
        state.getChannel(1).isEnabled() && isFlashActive(channelFlash[1]))
    {
      leds[currentPos] = CH2_COLOR;
    }
    else
    {
      leds[currentPos] = CRGB::Black;
    }
    return;
  }
  currentPos += BLINKER_SIZE;

  // CH2 pattern
  if (section == CH2_PATTERN)
  {
    drawPattern(state.getChannel(1), currentPos, MAX_PATTERN_SIZE, state, CH2_COLOR);
    return;
  }
  currentPos += state.getChannel(1).getBarLength();

  // BPM end
  if (section == BPM_END)
  {
    leds[currentPos] = (state.isRunning && !state.isPaused && isFlashActive(globalFlash))
                           ? CRGB::White
                           : CRGB::Black;
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
    FastLED.show(); // Add show after clearing to prevent ghosting
  }
  FastLED.clear(); // Ensure all LEDs are off
  FastLED.show();
}
