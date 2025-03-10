#ifndef METRONOME_TIMER_H
#define METRONOME_TIMER_H

#include <Arduino.h>
#include "MetronomeState.h"

class MetronomeTimer
{
private:
  hw_timer_t *timer = NULL;
  portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
  MetronomeState *state;

  // Beat trigger flags for main loop processing
  volatile bool beatTrigger = false;
  volatile uint8_t activeBeatChannel = 0;
  volatile BeatState activeBeatState = SILENT;

  // Callback function pointer for solenoid activation
  typedef void (*BeatCallback)(uint8_t channel, BeatState beatState);
  BeatCallback beatCallback = nullptr;

  // Global pointer for the ISR to access the instance
  static MetronomeTimer *_instance;

public:
  MetronomeTimer(MetronomeState *statePtr) : state(statePtr)
  {
    _instance = this;
  }

  ~MetronomeTimer()
  {
    stop();
    if (timer != NULL)
    {
      timerEnd(timer);
      timer = NULL;
    }
    if (_instance == this)
    {
      _instance = nullptr;
    }
  }

  // Initialize the timer without starting it
  void init()
  {
    // Clean up any existing timer
    if (timer != NULL)
    {
      timerAlarmDisable(timer);
      timerDetachInterrupt(timer);
      timerEnd(timer);
    }

    // Set up the timer (use timer 0, prescaler 80 for microsecond precision, count up)
    timer = timerBegin(0, 80, true);

    // Attach the interrupt handler
    timerAttachInterrupt(timer, &staticTimerISR, true);

    // Calculate initial interval but don't start yet
    uint32_t periodMicros = calculateTimerInterval();
    timerAlarmWrite(timer, periodMicros, true);
  }

  // Static wrapper for the timer ISR - this needs to be outside the class
  static void IRAM_ATTR staticTimerISR();

  // The actual interrupt handler implementation
  void IRAM_ATTR handleInterrupt()
  {
    // Use a mutex to prevent race conditions
    portENTER_CRITICAL_ISR(&timerMux);

    // Increment the global tick and update timing
    state->globalTick++;
    state->lastBeatTime = millis();

    // Check each channel to see if it should trigger on this tick
    for (uint8_t i = 0; i < 2; i++)
    {
      MetronomeChannel &channel = state->getChannel(i);
      if (channel.isEnabled())
      {
        // Only update the beat if the channel's timing aligns with the global tick
        channel.updateBeat();

        // Set flags for the main loop to handle the solenoid
        BeatState currentState = channel.getBeatState();
        if (currentState != SILENT)
        {
          beatTrigger = true;
          activeBeatChannel = i;
          activeBeatState = currentState;
        }
      }
    }

    portEXIT_CRITICAL_ISR(&timerMux);
  }

  // Calculate timer interval based on BPM
  uint32_t calculateTimerInterval()
  {
    uint32_t effectiveBpm = state->getEffectiveBpm();
    // Period in microseconds = (60 seconds / BPM) * 1,000,000
    return (60 * 1000000) / effectiveBpm;
  }

  // Start the metronome timer
  void start()
  {
    if (timer == NULL)
    {
      init();
    }

    // Reset timing variables for a clean start
    state->globalTick = 0;
    state->lastBeatTime = millis();

    // Reset channels to start at beginning
    for (uint8_t i = 0; i < 2; i++)
    {
      MetronomeChannel &channel = state->getChannel(i);
      channel.resetBeat();
    }

    // Calculate the proper interval based on current BPM
    uint32_t periodMicros = calculateTimerInterval();
    timerAlarmWrite(timer, periodMicros, true);

    // Enable the timer alarm
    timerAlarmEnable(timer);
  }

  // Stop the metronome timer
  void stop()
  {
    if (timer != NULL)
    {
      timerAlarmDisable(timer);
    }
  }

  // Update the timer interval (when BPM changes)
  void updateTiming()
  {
    if (timer != NULL)
    {
      uint32_t periodMicros = calculateTimerInterval();
      timerAlarmWrite(timer, periodMicros, true);
    }
  }

  // Register callback for beat events
  void setBeatCallback(BeatCallback callback)
  {
    beatCallback = callback;
  }

  // Process any pending beat events (call this from the main loop)
  void processBeat()
  {
    if (beatTrigger && beatCallback)
    {
      // Clear the flag first to avoid missing subsequent triggers
      portENTER_CRITICAL(&timerMux);
      beatTrigger = false;
      uint8_t channel = activeBeatChannel;
      BeatState beatState = activeBeatState;
      portEXIT_CRITICAL(&timerMux);

      // Call the callback with the channel and beat state
      beatCallback(channel, beatState);
    }
  }

  // Check if there's a pending beat to process
  bool hasPendingBeat()
  {
    return beatTrigger;
  }

  // Get the active beat state (for direct access if needed)
  BeatState getActiveBeatState()
  {
    portENTER_CRITICAL(&timerMux);
    BeatState state = activeBeatState;
    portEXIT_CRITICAL(&timerMux);
    return state;
  }

  // Get the active beat channel (for direct access if needed)
  uint8_t getActiveBeatChannel()
  {
    portENTER_CRITICAL(&timerMux);
    uint8_t channel = activeBeatChannel;
    portEXIT_CRITICAL(&timerMux);
    return channel;
  }
};

// Initialize static member
MetronomeTimer *MetronomeTimer::_instance = nullptr;

// Define the static ISR outside the class
void IRAM_ATTR MetronomeTimer::staticTimerISR()
{
  if (_instance)
  {
    _instance->handleInterrupt();
  }
}

#endif // METRONOME_TIMER_H