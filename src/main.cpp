#include <Arduino.h>
#include "config.h"
#include "Display.h"
#include "MetronomeState.h"
#include "MetronomeTimer.h"
#include "SolenoidController.h"
#include "EncoderController.h"

MetronomeState state;
Display display;
MetronomeTimer metronomeTimer(&state);
SolenoidController solenoidController(SOLENOID_PIN);
EncoderController encoderController(state, metronomeTimer);

void onBeatEvent(uint8_t channel, BeatState beatState)
{
    solenoidController.processBeat(channel, beatState);
}

void setup()
{
    solenoidController.init();
    display.begin();
    encoderController.begin();
    metronomeTimer.setBeatCallback(onBeatEvent);
}

void loop()
{
    encoderController.handleControls();
    metronomeTimer.processBeat();
    display.update(state);

    // Prevent watchdog timeouts
    yield();
}