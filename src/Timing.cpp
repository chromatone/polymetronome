#include "Timing.h"
#include "SolenoidController.h"
#include "AudioController.h"
#include "Display.h"
#include "LEDController.h" // Add this include

// Initialize static instance pointer
Timing *Timing::instance = nullptr;

// Static callback wrappers
void Timing::onClockPulseStatic(uint32_t tick)
{
    if (instance)
    {
        instance->onClockPulse(tick);
    }
}

void Timing::onSync24Static(uint32_t tick)
{
    if (instance && instance->wirelessSync.isInitialized())
    {
        instance->wirelessSync.onSync24(tick);
    }
}

void Timing::onPPQNStatic(uint32_t tick)
{
    if (instance)
    {
        // Process main metronome logic first
        instance->onClockPulse(tick);

        // Then handle wireless sync if initialized
        if (instance->wirelessSync.isInitialized())
        {
            instance->wirelessSync.onPPQN(tick, instance->state);
        }
    }
}

void Timing::onStepStatic(uint32_t tick)
{
    if (instance && instance->wirelessSync.isInitialized())
    {
        instance->wirelessSync.onStep(tick, instance->state);
    }
}

Timing::Timing(MetronomeState &state,
               WirelessSync &wirelessSync,
               SolenoidController &solenoidController,
               AudioController &audioController)
    : state(state),
      wirelessSync(wirelessSync),
      solenoidController(solenoidController),
      audioController(audioController),
      display(nullptr)
{

    // Set the singleton instance
    instance = this;
}

void Timing::setDisplay(Display *displayRef)
{
    display = displayRef;
}

void Timing::setLEDController(LEDController *controller)
{
    ledController = controller;
}

void Timing::init()
{
    // Initialize uClock
    uClock.init();

    // Set uClock mode based on leader status
    if (wirelessSync.isInitialized())
    {
        if (wirelessSync.isLeader())
        {
            // Leader mode - internal clock
            uClock.setMode(uClock.INTERNAL_CLOCK);
        }
        else
        {
            // Follower mode - external clock
            uClock.setMode(uClock.EXTERNAL_CLOCK);
        }
    }
    else
    {
        // No wireless sync, use internal clock
        uClock.setMode(uClock.INTERNAL_CLOCK);
    }

    // Register callbacks
    uClock.setOnSync24(onSync24Static);
    uClock.setOnPPQN(onPPQNStatic);
    uClock.setOnStep(onStepStatic);

    uClock.setPPQN(uClock.PPQN_96);
    uClock.setTempo(state.bpm);
}

void Timing::update()
{
    // Check if running state has changed
    if (state.isRunning != previousRunningState)
    {
        previousRunningState = state.isRunning;

        // Handle state changes
        if (state.isRunning && !state.isPaused)
        {
            start();
        }
        else if (!state.isRunning)
        {
            if (state.isPaused)
            {
                pause();
            }
            else
            {
                stop();
            }
        }
    }
}

void Timing::onBeatEvent(uint8_t channel, BeatState beatState)
{
    solenoidController.processBeat(channel, beatState);
    audioController.processBeat(channel, beatState);

    // Trigger LED flash on active beats
    if (ledController && beatState != SILENT)
    {
        ledController->onChannelBeat(channel);
    }
}

void Timing::onClockPulse(uint32_t tick)
{
    // Update the fractional tick position on every pulse
    state.updateTickFraction(tick);

    // Trigger global beat flash on quarter notes
    if (tick % 96 == 0 && ledController)
    {
        ledController->onGlobalBeat();
    }

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
    // This is the ONLY place where channel 2 beats are processed in polyrhythm mode
    if (isPolyrhythm && state.getChannel(1).isEnabled())
    {
        // Check if this tick should trigger a beat for channel 2 based on its polyrhythm subdivision
        BeatState ch2State = state.getChannel(1).getPolyrhythmBeatState(tick, state);
        if (ch2State != SILENT)
        {
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
        if (!isPolyrhythm)
        {
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
        else
        {
            // In polyrhythm mode, ONLY handle channel 1 on quarter note boundaries
            // Channel 2 is handled above at the PPQN level for more precise timing
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
        }
    }
}

void Timing::start()
{
    // Start the display animation if available
    if (display)
    {
        display->startAnimation();
    }

    if (wirelessSync.isInitialized() && wirelessSync.isLeader())
    {
        wirelessSync.sendControl(CMD_START);
    }
    uClock.start();
}

void Timing::stop()
{
    if (wirelessSync.isInitialized() && wirelessSync.isLeader())
    {
        wirelessSync.sendControl(CMD_STOP);
    }
    uClock.stop();
}

void Timing::pause()
{
    if (wirelessSync.isInitialized() && wirelessSync.isLeader())
    {
        wirelessSync.sendControl(CMD_PAUSE);
    }
    uClock.pause();
}

void Timing::setTempo(uint16_t bpm)
{
    uClock.setTempo(bpm);
}