#include "LEDController.h"

#define LED_PIN 27 // Using GPIO27 for LED strip
#define NUM_LEDS 33
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

LEDController::LEDController() : leds(nullptr)
{
  mainSection = NUM_LEDS / 3;
  channelSection = mainSection;
}

LEDController::~LEDController()
{
  if (leds)
  {
    delete[] leds;
  }
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

void LEDController::update(const MetronomeState &state)
{
  // Update main beat section
  uint32_t now = millis();
  if (now - lastBeatTime < beatDuration)
  {
    // Flash white on beat
    for (int i = 0; i < mainSection; i++)
    {
      leds[i] = CRGB::White;
    }
  }
  else
  {
    // Clear main section
    for (int i = 0; i < mainSection; i++)
    {
      leds[i] = CRGB::Black;
    }
  }

  // Update channel sections
  for (int i = 0; i < FIXED_CHANNEL_COUNT; i++)
  {
    const MetronomeChannel &channel = state.getChannel(i);
    if (channel.isEnabled())
    {
      int startLed = mainSection + (i * channelSection);
      int endLed = startLed + channelSection;

      for (int led = startLed; led < endLed; led++)
      {
        uint8_t position = led - startLed;
        if (position == channel.getCurrentBeat())
        {
          leds[led] = channel.getPatternBit(position) ? CRGB::White : CRGB::Red;
        }
        else
        {
          leds[led] = CRGB::Black;
        }
      }
    }
  }

  FastLED.show();
}

void LEDController::onBeat(uint8_t channel, BeatState beatState)
{
  if (channel == 0)
  { // Only flash main section on channel 1 beats
    lastBeatTime = millis();
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
