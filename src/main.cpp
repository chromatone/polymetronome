#include <Arduino.h>
#include "config.h"
#include "Display.h"
#include "MetronomeState.h"
#include "MetronomeTimer.h"
#include "SolenoidController.h"
#include "AudioController.h"
#include "EncoderController.h"

MetronomeState state;
Display display;
MetronomeTimer metronomeTimer(&state);
SolenoidController solenoidController(SOLENOID_PIN);
AudioController audioController(DAC_PIN);
EncoderController encoderController(state, metronomeTimer);

// Track previous running state to detect changes
bool previousRunningState = false;

void onBeatEvent(uint8_t channel, BeatState beatState)
{
    solenoidController.processBeat(channel, beatState);
    audioController.processBeat(channel, beatState);
}

void setup()
{
    solenoidController.init();
    audioController.init();
    display.begin();
    encoderController.begin();
    metronomeTimer.setBeatCallback(onBeatEvent);
    
    // Start animation ticker for UI effects
    display.startAnimation();
}

void loop()
{
    // Check if running state has changed
    if (state.isRunning != previousRunningState) {
        previousRunningState = state.isRunning;
        
        // Reset animation ticker when state changes
        if (state.isRunning) {
            display.startAnimation();
        }
    }
    
    encoderController.handleControls();
    metronomeTimer.processBeat();
    display.update(state);

    // Prevent watchdog timeouts
    yield();
}