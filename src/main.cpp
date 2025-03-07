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
        // Toggle editing mode
        state.isEditing = !state.isEditing;
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
        
        if (state.isEditing) {
            if (state.isBpmSelected()) {
                state.bpm = constrain(state.bpm + diff, MIN_BPM, MAX_BPM);
            } 
            else {
                uint8_t channelIndex = state.getActiveChannel();
                bool isLength = state.menuPosition % 2 == 1;
                auto& channel = state.getChannel(channelIndex);
                
                if (isLength) {
                    channel.setBarLength(channel.getBarLength() + diff);
                } else {
                    uint16_t newPattern = channel.getPattern() + diff;
                    channel.setPattern(constrain(newPattern, 0, channel.getMaxPattern()));
                }
            }
        } else {
            // Navigate menu
            state.menuPosition = (state.menuPosition + state.getMenuItemsCount() + diff) % state.getMenuItemsCount();
        }
        lastEncoderValue = encoderValue;
    }
}

void updateMetronome() {
    if (state.isRunning) {
        state.update();
        
        // Handle solenoid actuation for active channels
        for (uint8_t i = 0; i < 2; i++) {
            const MetronomeChannel& channel = state.getChannel(i);
            if (channel.getBeatState() == ACCENT) {
                digitalWrite(SOLENOID_PIN, HIGH);
                delay(ACCENT_PULSE_MS);
                digitalWrite(SOLENOID_PIN, LOW);
            } else if (channel.getBeatState() == WEAK) {
                digitalWrite(SOLENOID_PIN, HIGH);
                delay(SOLENOID_PULSE_MS);
                digitalWrite(SOLENOID_PIN, LOW);
            }
        }
    }
}

void loop() {
    handleButtons();
    handleControls();
    updateMetronome();
    display.update(state);
}
