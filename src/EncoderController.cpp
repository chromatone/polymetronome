#include "EncoderController.h"
#include <uClock.h>

// Global pointer for ISR to access
EncoderController *globalEncoderController = nullptr;

// ISR function that calls the controller method
void IRAM_ATTR globalEncoderISR()
{
  if (globalEncoderController)
  {
    globalEncoderController->encoderISRHandler();
  }
}

EncoderController::EncoderController(MetronomeState &state)
    : state(state)
{
  globalEncoderController = this;
}

void EncoderController::begin()
{
  pinMode(ENCODER_A, INPUT_PULLUP);
  pinMode(ENCODER_B, INPUT_PULLUP);
  pinMode(ENCODER_BTN, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENCODER_A), globalEncoderISR, CHANGE);
}

void EncoderController::handleControls()
{
  handleEncoderButton();
  handleStartButton();
  handleStopButton();
  handleRotaryEncoder();
}

void EncoderController::encoderISRHandler()
{
  uint8_t a = digitalRead(ENCODER_A);
  uint8_t b = digitalRead(ENCODER_B);

  if (a != lastEncA)
  {
    lastEncA = a;
    encoderValue += (a != b) ? 1 : -1;
  }
}

void EncoderController::handleEncoderButton()
{
  bool encBtn = digitalRead(ENCODER_BTN);
  uint32_t currentTime = millis();

  // Button pressed
  if (encBtn == LOW && lastEncBtn == HIGH) {
    // Start timing for long press
    buttonPressStartTime = currentTime;
    buttonLongPressActive = false;
  }
  // Button released
  else if (encBtn == HIGH && lastEncBtn == LOW) {
    // If it wasn't a long press, handle as a normal click
    if (!buttonLongPressActive) {
      // Check if a channel toggle is selected
      for (uint8_t i = 0; i < MetronomeState::CHANNEL_COUNT; i++) {
        if (state.isToggleSelected(i)) {
          // Toggle the channel on/off
          state.getChannel(i).toggleEnabled();
          lastEncBtn = encBtn;
          return;
        }
      }
      
      // Otherwise toggle editing mode as before
      state.isEditing = !state.isEditing;
    }
    // Reset long press state on button release
    buttonLongPressActive = false;
  }
  // Button still pressed, check for long press
  else if (encBtn == LOW && !buttonLongPressActive) {
    // Check if we've held the button long enough for a long press
    if (currentTime - buttonPressStartTime > LONG_PRESS_DURATION_MS) {
      buttonLongPressActive = true;
      
      // Handle long press for Euclidean rhythm reset
      uint8_t channelIndex = state.getActiveChannel();
      
      // Only apply Euclidean rhythm if we're on a pattern
      if (state.isPatternSelected(channelIndex)) {
        auto &channel = state.getChannel(channelIndex);
        
        // Count active beats in current pattern
        uint16_t pattern = channel.getPattern();
        uint8_t barLength = channel.getBarLength();
        uint8_t activeBeats = 0;
        
        // Count active beats in the pattern
        // First beat is always active
        activeBeats = 1;
        
        // Count active beats in the rest of the pattern
        for (uint8_t i = 0; i < barLength - 1; i++) {
          if ((pattern >> i) & 1) {
            activeBeats++;
          }
        }
        
        // Debug output
        Serial.print("Active beats: ");
        Serial.print(activeBeats);
        Serial.print(" / Bar length: ");
        Serial.println(barLength);
        
        // Generate Euclidean rhythm with the same number of active beats
        channel.generateEuclidean(activeBeats);
        
        // Set a flag to show feedback
        state.euclideanApplied = true;
        state.euclideanAppliedTime = currentTime;
        
        // Exit editing mode
        state.isEditing = false;
      }
    }
  }
  
  lastEncBtn = encBtn;
}

void EncoderController::handleStartButton()
{
  bool startBtn = digitalRead(BTN_START);

  if (startBtn != lastStartBtn && startBtn == LOW)
  {
    // If stopped, start playback
    if (!state.isRunning && !state.isPaused) {
      state.isRunning = true;
      state.isPaused = false;
      uClock.start();
    } 
    // If running, pause playback
    else if (state.isRunning && !state.isPaused) {
      state.isRunning = false;
      state.isPaused = true;
      uClock.pause();
    }
    // If paused, resume playback
    else if (!state.isRunning && state.isPaused) {
      state.isRunning = true;
      state.isPaused = false;
      uClock.pause(); // uClock.pause() toggles between pause and resume
    }
  }
  lastStartBtn = startBtn;
}

void EncoderController::handleStopButton()
{
  bool stopBtn = digitalRead(BTN_STOP);

  if (stopBtn != lastStopBtn && stopBtn == LOW)
  {
    // Reset all state variables
    state.isRunning = false;
    state.isPaused = false;
    state.currentBeat = 0;
    state.globalTick = 0;
    state.lastBeatTime = 0;
    state.tickFraction = 0.0f;
    state.lastPpqnTick = 0;

    // Stop the clock
    uClock.stop();

    // Reset all channels
    for (uint8_t i = 0; i < MetronomeState::CHANNEL_COUNT; i++)
    {
      state.getChannel(i).resetBeat();
    }
    
    // Debug output
    Serial.println("Metronome stopped and reset");
  }
  lastStopBtn = stopBtn;
}

void EncoderController::handleRotaryEncoder()
{
  static int32_t lastEncoderValue = encoderValue;
  int32_t currentStep = encoderValue / 2;
  int32_t lastStep = lastEncoderValue / 2;

  if (currentStep == lastStep)
    return;

  int32_t diff = currentStep - lastStep;
  lastEncoderValue = encoderValue;

  if (state.isEditing)
  {
    if (state.isBpmSelected())
    {
      state.bpm = constrain(state.bpm + diff, MIN_GLOBAL_BPM, MAX_GLOBAL_BPM);
      uClock.setTempo(state.bpm);
    }
    else if (state.isMultiplierSelected())
    {
      state.adjustMultiplier(diff);
    }
    else
    {
      uint8_t channelIndex = state.getActiveChannel();
      auto &channel = state.getChannel(channelIndex);

      if (state.isLengthSelected(channelIndex))
      {
        channel.setBarLength(channel.getBarLength() + diff);
      }
      else if (state.isPatternSelected(channelIndex))
      {
        int newPattern = (static_cast<int>(channel.getPattern()) + channel.getMaxPattern() + 1 + diff) % (channel.getMaxPattern() + 1);
        channel.setPattern(static_cast<uint16_t>(newPattern));
      }
    }
  }
  else
  {
    int newPosition = (static_cast<int>(state.menuPosition) + state.getMenuItemsCount() + diff) % state.getMenuItemsCount();
    state.menuPosition = static_cast<MenuPosition>(newPosition);
  }
}