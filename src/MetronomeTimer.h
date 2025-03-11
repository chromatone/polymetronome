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

  static void IRAM_ATTR timerCallback();

public:
  MetronomeTimer(MetronomeState *statePtr) : state(statePtr) { _instance = this; }
  ~MetronomeTimer()
  {
    stop();
    _instance = (_instance == this) ? nullptr : _instance;
  }

  void start();
  void stop() { ticker.detach(); }
  void updateTiming();

  void IRAM_ATTR handleInterrupt();
  void setBeatCallback(BeatCallback callback) { beatCallback = callback; }
  void processBeat();

  bool hasPendingBeat() { return beatTrigger; }
  BeatState getActiveBeatState() { return activeBeatState; }
  uint8_t getActiveBeatChannel() { return activeBeatChannel; }
};

#endif // METRONOME_TIMER_H