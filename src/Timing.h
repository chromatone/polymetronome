#pragma once
#include <Arduino.h>
#include <uClock.h>
#include "MetronomeState.h"
#include "WirelessSync.h"
#include "BuzzerController.h" // Change from forward declaration to include

// Forward declarations
class SolenoidController;
class AudioController;
class Display;
class LEDController;

class Timing
{
private:
    MetronomeState &state;
    WirelessSync &wirelessSync;
    SolenoidController &solenoidController;
    AudioController &audioController;
    Display *display;
    LEDController *ledController;
    BuzzerController *buzzerController;

    // Track previous running state to detect changes
    bool previousRunningState = false;

    // Private callback handlers
    static void onClockPulseStatic(uint32_t tick);
    static void onSync24Static(uint32_t tick);
    static void onPPQNStatic(uint32_t tick);
    static void onStepStatic(uint32_t tick);

    // Pointer to the singleton instance for static callbacks
    static Timing *instance;

    // Process beat events
    void onBeatEvent(uint8_t channel, BeatState beatState);

public:
    Timing(MetronomeState &state,
           WirelessSync &wirelessSync,
           SolenoidController &solenoidController,
           AudioController &audioController,
           BuzzerController *buzzerCtrl)
        : state(state), wirelessSync(wirelessSync),
          solenoidController(solenoidController),
          audioController(audioController),
          buzzerController(buzzerCtrl)
    {
        instance = this;
    }

    // Set display reference
    void setDisplay(Display *displayRef);

    // Initialize timing system
    void init();

    // Update timing state
    void update();

    // Process clock pulse
    void onClockPulse(uint32_t tick);

    // Start/stop/pause playback
    void start();
    void stop();
    void pause();

    // Set tempo
    void setTempo(uint16_t bpm);

    // Set LED controller
    void setLEDController(LEDController *controller);
};