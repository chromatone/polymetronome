#include "Timing.h"
#include "SolenoidController.h"
#include "AudioController.h"
#include "Display.h"
#include "LEDController.h"
#include "BuzzerController.h"

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
    if (buzzerController)
    {
        buzzerController->processBeat(channel, beatState);
    }

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
    float multiplier = state.getCurrentMultiplier();
    uint32_t effectiveTick = uint32_t(tick * multiplier);

    // Convert PPQN ticks to quarter note beats
    uint32_t quarterNoteTick = effectiveTick / 96;

    // In polyrhythm mode, we need more frequent checks
    if (state.isPolyrhythm() && state.getChannel(1).isEnabled())
    {
        // Check for channel 2 beats more frequently for smoother polyrhythm
        MetronomeChannel &channel2 = state.getChannel(1);
        BeatState ch2State = channel2.getPolyrhythmBeatState(tick, state);

        if (ch2State != SILENT)
        {
            onBeatEvent(1, ch2State);
            // Update the current beat for visualization
            uint8_t ch1Length = state.getChannel(0).getBarLength();
            float cycleProgress = float(state.globalTick % ch1Length) / float(ch1Length);
            cycleProgress += state.tickFraction / float(ch1Length);
            float beatPosition = cycleProgress * float(channel2.getBarLength());
            channel2.updateBeat(uint32_t(beatPosition));
        }
    }

    // Process quarter note boundaries
    if (effectiveTick % 96 == 0)
    {
        state.globalTick = quarterNoteTick;
        state.lastBeatTime = quarterNoteTick;

        // Always process channel 1 on quarter notes
        MetronomeChannel &channel1 = state.getChannel(0);
        if (channel1.isEnabled())
        {
            channel1.updateBeat(quarterNoteTick);
            BeatState ch1State = channel1.getBeatState();
            if (ch1State != SILENT)
            {
                onBeatEvent(0, ch1State);
            }
        }

        // Only process channel 2 here if NOT in polyrhythm mode
        if (!state.isPolyrhythm())
        {
            MetronomeChannel &channel2 = state.getChannel(1);
            if (channel2.isEnabled())
            {
                channel2.updateBeat(quarterNoteTick);
                BeatState ch2State = channel2.getBeatState();
                if (ch2State != SILENT)
                {
                    onBeatEvent(1, ch2State);
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