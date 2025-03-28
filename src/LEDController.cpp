#include "LEDController.h"

#define LED_PIN 27 // Using GPIO27 for LED strip
#define NUM_LEDS 33
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

LEDController::LEDController()
    : leds(nullptr),
      numLeds(NUM_LEDS),
      mainSection(LEDS_PER_INDICATOR * (1 + FIXED_CHANNEL_COUNT)),
      channelSection((NUM_LEDS - (LEDS_PER_INDICATOR * (1 + FIXED_CHANNEL_COUNT))) / FIXED_CHANNEL_COUNT)
{
  // Constructor body can remain empty as initialization is done in init()
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
  clear();
  startupAnimation();
}

void LEDController::updateGlobalIndicator(const MetronomeState &state)
{
  // Global tempo indicator (first section)
  bool shouldShow = state.isRunning && !state.isPaused && isFlashActive(globalFlash);
  for (int i = 0; i < LEDS_PER_INDICATOR; i++)
  {
    leds[i] = shouldShow ? CRGB::White : CRGB::Black;
  }
}

void LEDController::updateChannelIndicators(const MetronomeState &state)
{
  for (int ch = 0; ch < FIXED_CHANNEL_COUNT; ch++)
  {
    const MetronomeChannel &channel = state.getChannel(ch);
    int startLed = LEDS_PER_INDICATOR * (1 + ch);

    bool shouldShow = state.isRunning && !state.isPaused &&
                      channel.isEnabled() && isFlashActive(channelFlash[ch]);

    for (int i = 0; i < LEDS_PER_INDICATOR; i++)
    {
      leds[startLed + i] = shouldShow ? CRGB::White : CRGB::Black;
    }
  }
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

CRGB LEDController::getPatternColor(const MetronomeChannel &channel, uint8_t position) const
{
  if (position == channel.getCurrentBeat())
  {
    return channel.getPatternBit(position) ? CRGB::White : CRGB::Red;
  }
  return CRGB::Black;
}

void LEDController::updateChannelPatterns(const MetronomeState &state)
{
  for (int ch = 0; ch < FIXED_CHANNEL_COUNT; ch++)
  {
    const MetronomeChannel &channel = state.getChannel(ch);
    if (channel.isEnabled())
    {
      int startLed = mainSection + (ch * channelSection);
      int endLed = startLed + channelSection;

      for (int led = startLed; led < endLed; led++)
      {
        uint8_t position = led - startLed;
        leds[led] = getPatternColor(channel, position);
      }
    }
  }
}

void LEDController::update(const MetronomeState &state)
{
  updateGlobalIndicator(state);
  updateChannelIndicators(state);
  updateChannelPatterns(state);
  FastLED.show();
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
