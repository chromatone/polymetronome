#include "MetronomeState.h"

MetronomeState::MetronomeState() : channels{MetronomeChannel(0), MetronomeChannel(1)}
{
  // Any additional initialization beyond the default values can go here
  // Most initializations are now handled by in-class member initializers
}

const MetronomeChannel &MetronomeState::getChannel(uint8_t index) const
{
  return channels[index];
}

MetronomeChannel &MetronomeState::getChannel(uint8_t index)
{
  return channels[index];
}

void MetronomeState::update()
{
  if (isRunning)
  {
    uint32_t currentTime = millis();
    if (lastBeatTime == 0)
    {
      lastBeatTime = currentTime;
    }

    // No beat timing logic here anymore - it's handled by the timer ISR

    // Just update progress for display purposes
    for (auto &channel : channels)
    {
      channel.updateProgress(currentTime, lastBeatTime, getEffectiveBpm());
    }
  }
}

uint8_t MetronomeState::getMenuItemsCount() const
{
  return 6;
}

uint8_t MetronomeState::getActiveChannel() const
{
  return (static_cast<uint8_t>(menuPosition) - MENU_CH1_LENGTH) / 2;
}

bool MetronomeState::isChannelSelected() const
{
  return static_cast<uint8_t>(menuPosition) > MENU_MULTIPLIER;
}

bool MetronomeState::isBpmSelected() const
{
  return navLevel == GLOBAL && menuPosition == MENU_BPM;
}

bool MetronomeState::isMultiplierSelected() const
{
  return navLevel == GLOBAL && menuPosition == MENU_MULTIPLIER;
}

bool MetronomeState::isLengthSelected(uint8_t channel) const
{
  return navLevel == GLOBAL &&
         menuPosition == static_cast<MenuPosition>(MENU_CH1_LENGTH + channel * 2);
}

bool MetronomeState::isPatternSelected(uint8_t channel) const
{
  return navLevel == GLOBAL &&
         menuPosition == static_cast<MenuPosition>(MENU_CH1_PATTERN + channel * 2);
}

float MetronomeState::getGlobalProgress() const
{
  if (!isRunning || !lastBeatTime)
    return 0.0f;
  uint32_t beatInterval = 60000 / getEffectiveBpm();
  return float(millis() - lastBeatTime) / beatInterval;
}

uint32_t MetronomeState::gcd(uint32_t a, uint32_t b) const
{
  while (b != 0)
  {
    uint32_t t = b;
    b = a % b;
    a = t;
  }
  return a;
}

uint32_t MetronomeState::lcm(uint32_t a, uint32_t b) const
{
  return (a * b) / gcd(a, b);
}

uint32_t MetronomeState::getTotalBeats() const
{
  uint32_t result = channels[0].getBarLength();
  for (uint8_t i = 1; i < CHANNEL_COUNT; i++)
  {
    result = lcm(result, channels[i].getBarLength());
  }
  return result;
}

float MetronomeState::getProgress() const
{
  if (!isRunning || !lastBeatTime)
    return 0.0f;
  uint32_t beatInterval = 60000 / getEffectiveBpm();
  uint32_t elapsed = millis() - lastBeatTime;
  return float(elapsed) / beatInterval;
}

float MetronomeState::getEffectiveBpm() const
{
  return bpm * multiplierValues[currentMultiplierIndex];
}

const char *MetronomeState::getCurrentMultiplierName() const
{
  return multiplierNames[currentMultiplierIndex];
}

void MetronomeState::adjustMultiplier(int8_t delta)
{
  currentMultiplierIndex = (currentMultiplierIndex + MULTIPLIER_COUNT + delta) % MULTIPLIER_COUNT;
}