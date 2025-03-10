#include <Arduino.h>
#include "config.h"
#include "Display.h"
#include "MetronomeState.h"

MetronomeState state;
Display display;

// Hardware timer declarations
hw_timer_t *metronomeTimer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

volatile int32_t encoderValue = 0;
volatile uint8_t lastEncA = HIGH;
volatile bool beatTrigger = false;
volatile uint8_t activeBeatChannel = 0;
volatile BeatState activeBeatState = SILENT;

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

// Timer interrupt handler - KEEP THIS AS SHORT AS POSSIBLE
void IRAM_ATTR onMetronomeTimer()
{
    // Use a mutex to prevent race conditions
    portENTER_CRITICAL_ISR(&timerMux);

    // Increment the global tick and set the beat trigger
    state.globalTick++;
    state.lastBeatTime = millis(); // Update for display purposes

    // Check each channel to see if it should trigger on this tick
    for (uint8_t i = 0; i < 2; i++)
    {
        MetronomeChannel &channel = state.getChannel(i);
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

// Function to set up the timer with the correct interval
void setupMetronomeTimer()
{
    // Stop the timer if it's already running
    if (metronomeTimer != NULL)
    {
        timerAlarmDisable(metronomeTimer);
        timerDetachInterrupt(metronomeTimer);
        timerEnd(metronomeTimer);
    }

    // Calculate the timer interval in microseconds based on effective BPM
    uint32_t effectiveBpm = state.getEffectiveBpm();

    // Period in microseconds = (60 seconds / BPM) * 1,000,000
    uint32_t periodMicros = (60 * 1000000) / effectiveBpm;

    // Set up the timer (use timer 0, prescaler 80 for microsecond precision, count up)
    metronomeTimer = timerBegin(0, 80, true);

    // Attach the interrupt handler
    timerAttachInterrupt(metronomeTimer, &onMetronomeTimer, true);

    // Set alarm to trigger at the calculated interval (autoreload = true)
    timerAlarmWrite(metronomeTimer, periodMicros, true);
}

void toggleMetronome(bool enable)
{
    if (enable)
    {
        // Set up the timer with the correct interval
        setupMetronomeTimer();

        // Reset timing variables for a clean start
        state.globalTick = 0;
        state.lastBeatTime = millis(); // Keep for display/UI purposes

        // Reset channels to start at beginning
        for (uint8_t i = 0; i < 2; i++)
        {
            MetronomeChannel &channel = state.getChannel(i);
            channel.resetBeat();
        }

        // Enable the timer alarm
        timerAlarmEnable(metronomeTimer);
    }
    else
    {
        // Disable the timer alarm
        if (metronomeTimer != NULL)
        {
            timerAlarmDisable(metronomeTimer);
        }
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
    pinMode(SOLENOID_PIN, OUTPUT);

    // Initialize display
    display.begin();

    // Set up encoder interrupt
    attachInterrupt(digitalPinToInterrupt(ENCODER_A), encoderISR, CHANGE);

    // Do not initialize the timer here - wait until start button is pressed

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
                toggleMetronome(false); // Stop first
                toggleMetronome(true);  // Then restart with new settings
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

// Function to handle solenoid actuation based on channel beat states
void handleSolenoid()
{
    // Only process if we have a trigger from the ISR
    if (beatTrigger)
    {
        // Clear the flag first to avoid missing subsequent triggers
        portENTER_CRITICAL(&timerMux);
        beatTrigger = false;
        BeatState beatState = activeBeatState;
        portEXIT_CRITICAL(&timerMux);

        if (beatState == ACCENT)
        {
            digitalWrite(SOLENOID_PIN, HIGH);
            delayMicroseconds(ACCENT_PULSE_MS * 1000);
            digitalWrite(SOLENOID_PIN, LOW);
            Serial.println("ACCENT beat");
        }
        else if (beatState == WEAK)
        {
            digitalWrite(SOLENOID_PIN, HIGH);
            delayMicroseconds(SOLENOID_PULSE_MS * 1000);
            digitalWrite(SOLENOID_PIN, LOW);
            Serial.println("WEAK beat");
        }
    }
}

void loop()
{
    static uint32_t lastDisplayUpdate = 0;
    uint32_t currentTime = millis();

    // Process inputs
    handleButtons();
    handleControls();

    // Process solenoid actuation from timer interrupt
    handleSolenoid();

    display.update(state);

    // Short yield to prevent watchdog timeouts
    yield();
}