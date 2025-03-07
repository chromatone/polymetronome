#include <Arduino.h>
#include "config.h"
#include "Display.h"
#include "MetronomeState.h"

MetronomeState state;
Display display;

volatile int32_t encoderValue = 0;
volatile uint8_t lastEncA = HIGH;

void IRAM_ATTR encoderISR() {
    uint8_t a = digitalRead(ENCODER_A);
    uint8_t b = digitalRead(ENCODER_B);
    
    if (a != lastEncA) {
        lastEncA = a;
        encoderValue += (a != b) ? 1 : -1;
    }
}

void setup() {
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
}

void handleButtons() {
    static bool lastEncBtn = HIGH;
    bool encBtn = digitalRead(ENCODER_BTN);
    
    if (encBtn != lastEncBtn && encBtn == LOW) {
        if (state.navLevel == GLOBAL && state.menuPosition > 0) {
            // Enter channel editing
            state.activeChannel = state.menuPosition - 1;
            state.navLevel = CHANNEL;
            state.menuPosition = 0;
        } else if (state.navLevel == CHANNEL) {
            // Exit to global
            state.navLevel = GLOBAL;
            state.menuPosition = state.activeChannel + 1;
        } else {
            // Toggle editing for BPM
            state.isEditing = !state.isEditing;
        }
    }
    lastEncBtn = encBtn;
    
    static bool lastStartBtn = HIGH;
    static bool lastStopBtn = HIGH;
    
    // Read current button states
    bool startBtn = digitalRead(BTN_START);
    bool stopBtn = digitalRead(BTN_STOP);
    
    // Handle start button
    if (startBtn != lastStartBtn && startBtn == LOW) {
        state.isRunning = true;
        state.lastBeatTime = millis();
    }
    lastStartBtn = startBtn;
    
    // Handle stop button
    if (stopBtn != lastStopBtn && stopBtn == LOW) {
        state.isRunning = false;
        state.currentBeat = 0;
    }
    lastStopBtn = stopBtn;
}

void handleControls() {
    static int32_t lastEncoderValue = encoderValue;
    int32_t currentStep = encoderValue / 2;
    int32_t lastStep = lastEncoderValue / 2;

    if (currentStep != lastStep) {
        int32_t diff = currentStep - lastStep;
        
        if (state.isEditing && state.navLevel == GLOBAL && state.menuPosition == 0) {
            // Edit BPM
            state.bpm = constrain(state.bpm + diff, MIN_BPM, MAX_BPM);
        } else if (state.navLevel == CHANNEL) {
            auto& channel = state.getChannel(state.activeChannel);
            switch(state.menuPosition) {
                case 0: // Bar Length
                    channel.setBarLength(channel.getBarLength() + diff);
                    break;
                case 1: // Pattern
                    channel.setPattern(channel.getPattern() + diff);
                    break;
            }
        } else {
            // Navigate menu
            if (state.navLevel == GLOBAL) {
                state.menuPosition = (state.menuPosition + diff + 3) % 3;
            } else {
                state.menuPosition = (state.menuPosition + diff + 2) % 2;
            }
        }
        lastEncoderValue = encoderValue;
    }
}

void updateMetronome() {
    if (state.isRunning) {
        // Individual channel updates are now handled in state.update()
        state.update();
        
        // Handle solenoid actuation for active channel
        const MetronomeChannel& activeChannel = state.getChannel(state.activeChannel);
        if (activeChannel.getBeatState()) {
            digitalWrite(SOLENOID_PIN, HIGH);
            delay(SOLENOID_PULSE_MS);
            digitalWrite(SOLENOID_PIN, LOW);
        }
    }
}

void loop() {
    handleButtons();
    handleControls();
    updateMetronome();
    display.update(state);
}
