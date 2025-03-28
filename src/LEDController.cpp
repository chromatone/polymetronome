#include "LEDController.h"

#define LED_PIN 27 // Using GPIO27 for LED strip
#define NUM_LEDS 33
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

LEDController::LEDController()
    : leds(nullptr),
      numLeds(NUM_LEDS)
{
  // Calculate section positions
  for (int i = 0; i < SECTION_COUNT; i++)
  {
    if (i == CH1_PATTERN || i == CH2_PATTERN)
    {
      sectionSizes[i] = PATTERN_SECTION_SIZE;
    }
    else
    {
      sectionSizes[i] = LEDS_PER_INDICATOR;
    }

    sectionStarts[i] = (i == 0) ? 0 : sectionStarts[i - 1] + sectionSizes[i - 1];
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

void LEDController::updateSection(StripSection section, const MetronomeState &state)
{
  uint8_t start = sectionStarts[section];
  uint8_t size = sectionSizes[section];

  switch (section)
  {
  case CH1_BLINK:
    if (state.isRunning && !state.isPaused &&
        state.getChannel(0).isEnabled() && isFlashActive(channelFlash[0]))
    {
      fill_solid(leds + start, size, CRGB::White);
    }
    else
    {
      fill_solid(leds + start, size, CRGB::Black);
    }
    break;

  case CH2_BLINK:
    if (state.isRunning && !state.isPaused &&
        state.getChannel(1).isEnabled() && isFlashActive(channelFlash[1]))
    {
      fill_solid(leds + start, size, CRGB::White);
    }
    else
    {
      fill_solid(leds + start, size, CRGB::Black);
    }
    break;

  case GLOBAL_BPM:
    if (state.isRunning && !state.isPaused && isFlashActive(globalFlash))
    {
      fill_solid(leds + start, size, CRGB::White);
    }
    else
    {
      fill_solid(leds + start, size, CRGB::Black);
    }
    break;

  case CH1_PATTERN:
  case CH2_PATTERN:
  {
    uint8_t ch = (section == CH1_PATTERN) ? 0 : 1;
    const MetronomeChannel &channel = state.getChannel(ch);

    // Get number of positions to show based on bar length
    uint8_t visiblePositions = min(size, channel.getBarLength());

    // Calculate spacing to distribute positions evenly
    float spacing = float(size) / float(visiblePositions);

    // Fill background
    fill_solid(leds + start, size, CRGB::Black);

    // Draw pattern positions
    if (channel.isEnabled())
    {
      for (uint8_t i = 0; i < visiblePositions; i++)
      {
        uint8_t ledPos = uint8_t(i * spacing);
        if (ledPos < size)
        {
          // Pass state to getPatternColor
          leds[start + ledPos] = getPatternColor(channel, i, state);
        }
      }
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
        return CRGB::White;  // Active beat, current position
    }
    else if (isCurrentBeat)
    {
        return CRGB::Red;    // Inactive beat, current position
    }
    else if (channel.getPatternBit(position))
    {
        return CRGB(32, 32, 32);  // Active beat, not current
    }

    return CRGB::Black;  // Inactive beat, not current
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
