#ifndef METRONOME_TIMER_H
#define METRONOME_TIMER_H

#include <Arduino.h>
#include <Ticker.h>
#include "MetronomeState.h"

class MetronomeTimer
{
private:
  Ticker ticker;
  MetronomeState *state;

  volatile bool beatTrigger = false;
  volatile uint8_t activeBeatChannel = 0;
  volatile BeatState activeBeatState = SILENT;

  typedef void (*BeatCallback)(uint8_t channel, BeatState beatState);
  BeatCallback beatCallback = nullptr;

  static MetronomeTimer *_instance;
  static void IRAM_ATTR timerCallback(); // Just declaration, implementation in cpp

public:
  MetronomeTimer(MetronomeState *statePtr) : state(statePtr) { _instance = this; }
  ~MetronomeTimer()
  {
    stop();
    _instance = (_instance == this) ? nullptr : _instance;
  }

  void start() {
    ticker.detach();

    state->globalTick = 0;
    // No need to use millis() for lastBeatTime as we're using ticks for timing
    state->lastBeatTime = 0;

    for (uint8_t i = 0; i < MetronomeState::CHANNEL_COUNT; i++) {
      state->getChannel(i).resetBeat();
    }
    
    // Immediately trigger the first beat for all channels
    for (uint8_t i = 0; i < MetronomeState::CHANNEL_COUNT; i++) {
      MetronomeChannel &channel = state->getChannel(i);
      if (channel.isEnabled()) {
        // Check if the first beat should be active
        BeatState currentState = channel.getBeatState();
        if (currentState != SILENT) {
          beatTrigger = true;
          activeBeatChannel = i;
          activeBeatState = currentState;
        }
      }
    }

    float periodSeconds = 60.0f / state->getEffectiveBpm();
    ticker.attach(periodSeconds, timerCallback);
  }
  
  void stop() { ticker.detach(); }
  
  void updateTiming() {
    if (ticker.active()) {
      float periodSeconds = 60.0f / state->getEffectiveBpm();
      ticker.detach();
      ticker.attach(periodSeconds, timerCallback);
    }
  }

  void IRAM_ATTR handleInterrupt() {
    state->globalTick++;
    // No need to use millis() for lastBeatTime as we're using ticks for timing
    // This avoids potential issues with millis() in ISRs
    state->lastBeatTime = state->globalTick;

    for (uint8_t i = 0; i < MetronomeState::CHANNEL_COUNT; i++) {
      MetronomeChannel &channel = state->getChannel(i);
      if (channel.isEnabled()) {
        channel.updateBeat();

        BeatState currentState = channel.getBeatState();
        if (currentState != SILENT) {
          beatTrigger = true;
          activeBeatChannel = i;
          activeBeatState = currentState;
        }
      }
    }
  }
  
  void setBeatCallback(BeatCallback callback) { beatCallback = callback; }
  
  void processBeat() {
    if (beatTrigger && beatCallback) {
      uint8_t channel = activeBeatChannel;
      BeatState state = activeBeatState;
      beatTrigger = false;

      beatCallback(channel, state);
    }
  }

  bool hasPendingBeat() { return beatTrigger; }
  BeatState getActiveBeatState() { return activeBeatState; }
  uint8_t getActiveBeatChannel() { return activeBeatChannel; }
};

#endif // METRONOME_TIMER_H