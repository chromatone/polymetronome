#include <Arduino.h>
#include <uClock.h>
#include "config.h"
#ifdef MAX_BPM
#undef MAX_BPM
#define MAX_BPM MAX_GLOBAL_BPM
#endif
#include "Display.h"
#include "MetronomeState.h"
#include "SolenoidController.h"
#include "AudioController.h"
#include "EncoderController.h"
#include "WirelessSync.h"

MetronomeState state;
Display display;
SolenoidController solenoidController(SOLENOID_PIN, SOLENOID_PIN2);
AudioController audioController(DAC_PIN);
EncoderController encoderController(state);
WirelessSync wirelessSync;

// Track previous running state to detect changes
bool previousRunningState = false;

void onBeatEvent(uint8_t channel, BeatState beatState)
{
    solenoidController.processBeat(channel, beatState);
    audioController.processBeat(channel, beatState);
}

void onClockPulse(uint32_t tick)
{
    // Update the fractional tick position on every pulse
    state.updateTickFraction(tick);
    
    // If paused, don't process clock pulses further
    if (state.isPaused)
        return;
        
    // Calculate effective tick based on multiplier
    uint32_t effectiveTick = tick * state.getCurrentMultiplier();

    // Convert PPQN ticks to quarter note beats
    uint32_t quarterNoteTick = effectiveTick / 96;

    // Only process on quarter note boundaries
    if (effectiveTick % 96 == 0)
    {
        state.globalTick = quarterNoteTick;
        state.lastBeatTime = quarterNoteTick;

        for (uint8_t i = 0; i < MetronomeState::CHANNEL_COUNT; i++)
        {
            MetronomeChannel &channel = state.getChannel(i);
            if (channel.isEnabled())
            {
                channel.updateBeat(quarterNoteTick);

                BeatState currentState = channel.getBeatState();
                if (currentState != SILENT)
                {
                    onBeatEvent(i, currentState);
                }
            }
        }
    }
}

// Wrapper functions for uClock callbacks
void onSync24Wrapper(uint32_t tick) {
    wirelessSync.onSync24(tick);
}

void onPPQNWrapper(uint32_t tick) {
    wirelessSync.onPPQN(tick, state);
}

void onStepWrapper(uint32_t step) {
    wirelessSync.onStep(step, state);
}

void setup()
{
    Serial.begin(115200);
    solenoidController.init();
    audioController.init();
    display.begin();
    encoderController.begin();

    // Initialize wireless sync
    if (wirelessSync.init()) {
        // Set a random priority based on device ID
        uint8_t devicePriority = random(1, 100);
        wirelessSync.setPriority(devicePriority);
        
        // Start leader negotiation
        wirelessSync.negotiateLeadership();
    }

    // Initialize uClock
    uClock.init();
    
    // Set uClock mode based on leader status
    if (wirelessSync.isLeader()) {
        // Leader mode - internal clock
        uClock.setMode(uClock.INTERNAL_CLOCK);
    } else {
        // Follower mode - external clock
        uClock.setMode(uClock.EXTERNAL_CLOCK);
    }
    
    // Register callbacks
    uClock.setOnSync24(onSync24Wrapper);
    uClock.setOnPPQN(onClockPulse); // Main metronome logic
    uClock.setOnStep(onStepWrapper);
    
    // Register wireless sync callbacks
    uClock.setOnSync24(onSync24Wrapper);
    uClock.setOnPPQN(onPPQNWrapper);
    
    uClock.setPPQN(uClock.PPQN_96);
    uClock.setTempo(state.bpm);
    
    // Start animation immediately (even before playback starts)
    display.startAnimation();
}

void loop()
{
    // Check if running state has changed
    if ((state.isRunning != previousRunningState) && !state.isPaused)
    {
        previousRunningState = state.isRunning;

        // Reset animation ticker when state changes (but not when pausing)
        if (state.isRunning)
        {
            display.startAnimation();
            wirelessSync.sendControl(CMD_START);
        }
        else
        {
            wirelessSync.sendControl(CMD_STOP);
        }
    }
    
    // Make sure animation is always running
    if (!display.isAnimationRunning())
    {
        display.startAnimation();
    }

    encoderController.handleControls();
    
    // Update state
    state.update();

    // Update wireless sync
    wirelessSync.update(state);
    
    // Check leader status periodically
    wirelessSync.checkLeaderStatus();

    display.update(state);
    // Prevent watchdog timeouts
    yield();
}