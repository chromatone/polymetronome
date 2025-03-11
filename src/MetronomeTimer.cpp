#include "MetronomeTimer.h"

MetronomeTimer *MetronomeTimer::_instance = nullptr;

void IRAM_ATTR MetronomeTimer::timerCallback()
{
  if (_instance)
    _instance->handleInterrupt();
}

void IRAM_ATTR MetronomeTimer::handleInterrupt()
{

  state->globalTick++;
  state->lastBeatTime = millis();

  for (uint8_t i = 0; i < 2; i++)
  {
    MetronomeChannel &channel = state->getChannel(i);
    if (channel.isEnabled())
    {
      channel.updateBeat();

      BeatState currentState = channel.getBeatState();
      if (currentState != SILENT)
      {
        beatTrigger = true;
        activeBeatChannel = i;
        activeBeatState = currentState;
      }
    }
  }
}

void MetronomeTimer::start()
{
  ticker.detach();

  state->globalTick = 0;
  state->lastBeatTime = millis();

  for (uint8_t i = 0; i < 2; i++)
  {
    state->getChannel(i).resetBeat();
  }

  float periodSeconds = 60.0f / state->getEffectiveBpm();
  ticker.attach(periodSeconds, timerCallback);
}

void MetronomeTimer::updateTiming()
{
  if (ticker.active())
  {
    float periodSeconds = 60.0f / state->getEffectiveBpm();
    ticker.detach();
    ticker.attach(periodSeconds, timerCallback);
  }
}

void MetronomeTimer::processBeat()
{
  if (beatTrigger && beatCallback)
  {
    uint8_t channel = activeBeatChannel;
    BeatState state = activeBeatState;
    beatTrigger = false;

    beatCallback(channel, state);
  }
}