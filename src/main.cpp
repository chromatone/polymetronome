#include <Arduino.h>
#include "config.h"
#include "Display.h"
#include "MetronomeState.h"
#include "MetronomeTimer.h"
#include "SolenoidController.h"

MetronomeState state;
Display display;
MetronomeTimer metronomeTimer(&state);
SolenoidController solenoidController(SOLENOID_PIN);

volatile int32_t encoderValue = 0;
volatile uint8_t lastEncA = HIGH;

void IRAM_ATTR encoderISR()
{
    uint8_t a = digitalRead(ENCODER_A);
    uint8_t b = digitalRead(ENCODER_B);

    if (a != lastEncA)
    {
        lastEncA = a;
        encoderValue += (a != b) ? 1 : -1;
    }
}

// Callback for beat events from the timer
void onBeatEvent(uint8_t channel, BeatState beatState)
{
    solenoidController.processBeat(channel, beatState);
}

void toggleMetronome(bool enable)
{
    if (enable)
    {
        metronomeTimer.start();
    }
    else
    {
        metronomeTimer.stop();
    }
}

void setup()
{
    // Initialize serial for debugging
    Serial.begin(115200);
    Serial.println("Metronome starting up");

    // Initialize pins
    pinMode(ENCODER_A, INPUT_PULLUP);
    pinMode(ENCODER_B, INPUT_PULLUP);
    pinMode(ENCODER_BTN, INPUT_PULLUP);
    pinMode(BTN_START, INPUT_PULLUP);
    pinMode(BTN_STOP, INPUT_PULLUP);

    // Initialize solenoid controller
    solenoidController.init();

    // Initialize display
    display.begin();

    // Initialize metronome timer
    metronomeTimer.init();
    metronomeTimer.setBeatCallback(onBeatEvent);

    // Set up encoder interrupt
    attachInterrupt(digitalPinToInterrupt(ENCODER_A), encoderISR, CHANGE);

    Serial.println("Setup complete");
}

void handleButtons()
{
    static bool lastEncBtn = HIGH;
    bool encBtn = digitalRead(ENCODER_BTN);

    if (encBtn != lastEncBtn && encBtn == LOW)
    {
        // Toggle editing mode
        state.isEditing = !state.isEditing;
        Serial.println(state.isEditing ? "Editing ON" : "Editing OFF");
    }
    lastEncBtn = encBtn;

    static bool lastStartBtn = HIGH;
    static bool lastStopBtn = HIGH;

    // Read current button states
    bool startBtn = digitalRead(BTN_START);
    bool stopBtn = digitalRead(BTN_STOP);

    // Handle start/pause button
    if (startBtn != lastStartBtn && startBtn == LOW)
    {
        // Toggle between play and pause
        if (state.isRunning)
        {
            // If currently running, pause it
            state.isRunning = false;
            state.isPaused = true;

            // Disable the timer
            toggleMetronome(false);
            Serial.println("Metronome paused");
        }
        else
        {
            // If stopped or paused, start it
            state.isRunning = true;
            state.isPaused = false;

            // Enable the timer
            toggleMetronome(true);
            Serial.println("Metronome started");
        }
    }
    lastStartBtn = startBtn;

    // Handle stop button - full reset
    if (stopBtn != lastStopBtn && stopBtn == LOW)
    {
        state.isRunning = false;
        state.isPaused = false; // Not paused, fully stopped
        state.currentBeat = 0;
        state.globalTick = 0;
        state.lastBeatTime = 0;

        // Disable the timer
        toggleMetronome(false);

        // Reset each channel
        for (uint8_t i = 0; i < 2; i++)
        {
            MetronomeChannel &channel = state.getChannel(i);
            channel.resetBeat();
        }

        Serial.println("Metronome stopped");
    }
    lastStopBtn = stopBtn;
}

void handleControls()
{
    static int32_t lastEncoderValue = encoderValue;
    int32_t currentStep = encoderValue / 2;
    int32_t lastStep = lastEncoderValue / 2;

    if (currentStep != lastStep)
    {
        int32_t diff = currentStep - lastStep;

        if (state.isEditing)
        {
            bool needTimerUpdate = false;

            if (state.isBpmSelected())
            {
                state.bpm = constrain(state.bpm + diff, MIN_BPM, MAX_BPM);
                needTimerUpdate = true;
                Serial.printf("BPM changed to: %d\n", state.bpm);
            }
            else if (state.isMultiplierSelected())
            {
                state.adjustMultiplier(diff);
                needTimerUpdate = true;
                Serial.printf("Multiplier changed to: %s\n", state.getCurrentMultiplierName());
            }
            else
            {
                uint8_t channelIndex = state.getActiveChannel();
                auto &channel = state.getChannel(channelIndex);

                if (state.isLengthSelected(channelIndex))
                {
                    channel.setBarLength(channel.getBarLength() + diff);
                    Serial.printf("Channel %d length changed to: %d\n", channelIndex, channel.getBarLength());
                }
                else if (state.isPatternSelected(channelIndex))
                {
                    uint16_t newPattern = channel.getPattern() + diff;
                    channel.setPattern(constrain(newPattern, 0, channel.getMaxPattern()));
                    Serial.printf("Channel %d pattern changed to: %d\n", channelIndex, channel.getPattern());
                }
            }

            // Update timer settings if BPM or multiplier changed
            if (needTimerUpdate && state.isRunning)
            {
                metronomeTimer.updateTiming();
                Serial.println("Timer reconfigured");
            }
        }
        else
        {
            // Navigate menu
            state.menuPosition = (state.menuPosition + state.getMenuItemsCount() + diff) % state.getMenuItemsCount();
            Serial.printf("Menu position: %d\n", state.menuPosition);
        }
        lastEncoderValue = encoderValue;
    }
}

void loop()
{
    // Process inputs
    handleButtons();
    handleControls();

    // Process any pending beat events from the timer interrupt
    metronomeTimer.processBeat();

    // Update the display
    display.update(state);

    // Short yield to prevent watchdog timeouts
    yield();
}