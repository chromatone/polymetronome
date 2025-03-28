#include "LEDController.h"

#define LED_PIN 27
#define NUM_LEDS 33
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

LEDController::LEDController() : leds(nullptr), numLeds(NUM_LEDS)
{
  globalFlash = {0, false};
  for (int i = 0; i < FIXED_CHANNEL_COUNT; i++)
  {
    channelFlash[i] = {0, false};
  }
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
  return flash.isFlashing && (millis() - flash.startTime) < LED_BEAT_DURATION_MS;
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
  clear();
  startupAnimation();
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

  fill_solid(leds + startLed, size, CRGB::Black);

  uint8_t currentBeat;
  if (channel.getId() == 1 && state.isPolyrhythm())
  {
    uint8_t ch1Length = state.getChannel(0).getBarLength();
    float cycleProgress = float(state.globalTick % ch1Length) / float(ch1Length);
    cycleProgress += state.tickFraction / float(ch1Length);
    currentBeat = uint8_t(cycleProgress * float(channel.getBarLength())) % channel.getBarLength();
  }
  else
  {
    currentBeat = channel.getCurrentBeat();
  }

  uint8_t patternLength = channel.getBarLength();
  for (uint8_t i = 0; i < patternLength && i < size; i++)
  {
    bool isActive = channel.getPatternBit(i);
    bool isCurrent = (i == currentBeat);

    CRGB color = CRGB::Black;
    if (isCurrent && isActive)
      color = CRGB::White;
    else if (isActive)
      color = baseColor;
    else if (isCurrent)
      color = CRGB(baseColor).nscale8(64);
    else
      color = CRGB(baseColor).nscale8(25);

    leds[startLed + i] = color;
  }
}

uint8_t LEDController::calculatePatternSpace(uint8_t barLength) const
{
  return min(barLength, MAX_PATTERN_SIZE);
}

void LEDController::updateSection(StripSection section, const MetronomeState &state)
{
  uint8_t currentPos = 0;
  bool isActive = state.isRunning && !state.isPaused;

  switch (section)
  {
  case BPM_START:
    leds[0] = isActive && isFlashActive(globalFlash) ? CRGB::White : CRGB::Black;
    break;

  case CH1_BLINK:
    leds[1] = isActive && state.getChannel(0).isEnabled() &&
                      isFlashActive(channelFlash[0])
                  ? CH1_COLOR
                  : CRGB::Black;
    break;

  case CH1_PATTERN:
    drawPattern(state.getChannel(0), 2, calculatePatternSpace(state.getChannel(0).getBarLength()),
                state, CH1_COLOR);
    break;

  case BPM_MID:
    currentPos = 2 + calculatePatternSpace(state.getChannel(0).getBarLength());
    leds[currentPos] = isActive && isFlashActive(globalFlash) ? CRGB::White : CRGB::Black;
    break;

  case CH2_BLINK:
    currentPos = 3 + calculatePatternSpace(state.getChannel(0).getBarLength());
    leds[currentPos] = isActive && state.getChannel(1).isEnabled() &&
                               isFlashActive(channelFlash[1])
                           ? CH2_COLOR
                           : CRGB::Black;
    break;

  case CH2_PATTERN:
    currentPos = 4 + calculatePatternSpace(state.getChannel(0).getBarLength());
    drawPattern(state.getChannel(1), currentPos, MAX_PATTERN_SIZE, state, CH2_COLOR);
    break;

  case BPM_END:
    currentPos = 4 + calculatePatternSpace(state.getChannel(0).getBarLength()) +
                 calculatePatternSpace(state.getChannel(1).getBarLength());
    if (currentPos < NUM_LEDS)
    {
      leds[currentPos] = isActive && isFlashActive(globalFlash) ? CRGB::White : CRGB::Black;
    }
    break;
  }
}

void LEDController::update(const MetronomeState &state)
{
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
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = CRGB::Green;
    FastLED.show();
    delay(20);
    leds[i] = CRGB::Black;
    FastLED.show();
  }
  FastLED.clear();
  FastLED.show();
}
