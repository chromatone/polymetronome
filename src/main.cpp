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

// Global pointer to WirelessSync instance for pattern change notifications
WirelessSync* globalWirelessSync = &wirelessSync;

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
    
    // Always store the last PPQN tick for polyrhythm calculations
    state.lastPpqnTick = tick;
    
    // If paused, don't process clock pulses further
    if (state.isPaused)
        return;
        
    // Calculate effective tick based on multiplier
    uint32_t effectiveTick = tick * state.getCurrentMultiplier();

    // Convert PPQN ticks to quarter note beats
    uint32_t quarterNoteTick = effectiveTick / 96;

    // In polyrhythm mode, we need more frequent checks to catch all subdivisions
    bool isPolyrhythm = state.isPolyrhythm();
    
    // For polyrhythm, use the precise timing method for channel 2
    if (isPolyrhythm && state.getChannel(1).isEnabled()) {
        // Check if this tick should trigger a beat for channel 2 based on its polyrhythm subdivision
        BeatState ch2State = state.getChannel(1).getPolyrhythmBeatState(tick, state);
        if (ch2State != SILENT) {
            // Trigger the beat event for channel 2
            onBeatEvent(1, ch2State);
        }
    }

    // Only process quarter note boundaries for channel 1 and for polymeter mode
    if (effectiveTick % 96 == 0)
    {
        state.globalTick = quarterNoteTick;
        state.lastBeatTime = quarterNoteTick;

        // In polymeter mode, handle both channels
        if (!isPolyrhythm) {
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
        } else {
            // In polyrhythm mode, only handle channel 1 on quarter note boundaries
            MetronomeChannel &channel1 = state.getChannel(0);
            if (channel1.isEnabled())
            {
                channel1.updateBeat(quarterNoteTick);

                BeatState currentState = channel1.getBeatState();
                if (currentState != SILENT)
                {
                    onBeatEvent(0, currentState);
                }
            }
            
            // We don't need to update polyrhythm beat position for channel 2 here
            // since it's already handled by the PPQN-based mechanism above
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
    
    // Register callbacks - only register each callback once
    uClock.setOnSync24(onSync24Wrapper);
    uClock.setOnPPQN([](uint32_t tick) {
        // Process main metronome logic first
        onClockPulse(tick);
        // Then handle wireless sync
        wirelessSync.onPPQN(tick, state);
    });
    uClock.setOnStep(onStepWrapper);
    
    uClock.setPPQN(uClock.PPQN_96);
    uClock.setTempo(state.bpm);
    
    // Start animation immediately (even before playback starts)
    display.startAnimation();
}

void loop()
{
    // Check if running state has changed
    if (state.isRunning != previousRunningState)
    {
        previousRunningState = state.isRunning;

        // Reset animation ticker when state changes (but not when pausing)
        if (state.isRunning && !state.isPaused)
        {
            display.startAnimation();
            if (wirelessSync.isLeader()) {
                wirelessSync.sendControl(CMD_START);
            }
            uClock.start();
        }
        else if (!state.isRunning)
        {
            if (wirelessSync.isLeader()) {
                wirelessSync.sendControl(state.isPaused ? CMD_PAUSE : CMD_STOP);
            }
            if (!state.isPaused) {
                uClock.stop();
            } else {
                uClock.pause();
            }
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