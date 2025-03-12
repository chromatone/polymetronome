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

MetronomeState state;
Display display;
SolenoidController solenoidController(SOLENOID_PIN, SOLENOID_PIN2);
AudioController audioController(DAC_PIN);
EncoderController encoderController(state);

// Track previous running state to detect changes
bool previousRunningState = false;

void onBeatEvent(uint8_t channel, BeatState beatState)
{
    solenoidController.processBeat(channel, beatState);
    audioController.processBeat(channel, beatState);
}

void onClockPulse(uint32_t tick)
{
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

void setup()
{
    Serial.begin(115200);
    solenoidController.init();
    audioController.init();
    display.begin();
    encoderController.begin();

    // Initialize uClock
    uClock.init();
    uClock.setOnPPQN(onClockPulse);
    uClock.setPPQN(uClock.PPQN_96);
    uClock.setTempo(state.getEffectiveBpm());

    // Start animation ticker for UI effects
    display.startAnimation();
}

void loop()
{
    // Check if running state has changed
    if (state.isRunning != previousRunningState)
    {
        previousRunningState = state.isRunning;

        // Reset animation ticker when state changes
        if (state.isRunning)
        {
            display.startAnimation();
        }
    }

    encoderController.handleControls();

    display.update(state);
    // Prevent watchdog timeouts
    yield();
}