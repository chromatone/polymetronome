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

  if (encBtn != lastEncBtn && encBtn == LOW)
  {
    state.isEditing = !state.isEditing;
  }
  lastEncBtn = encBtn;
}

void EncoderController::handleStartButton()
{
  bool startBtn = digitalRead(BTN_START);

  if (startBtn != lastStartBtn && startBtn == LOW)
  {
    state.isRunning = !state.isRunning;
    state.isPaused = !state.isRunning;

    if (state.isRunning)
    {
      uClock.start();
    }
    else
    {
      uClock.stop();
    }
  }
  lastStartBtn = startBtn;
}

void EncoderController::handleStopButton()
{
  bool stopBtn = digitalRead(BTN_STOP);

  if (stopBtn != lastStopBtn && stopBtn == LOW)
  {
    state.isRunning = false;
    state.isPaused = false;
    state.currentBeat = 0;
    state.globalTick = 0;
    state.lastBeatTime = 0;

    uClock.stop();

    for (uint8_t i = 0; i < MetronomeState::CHANNEL_COUNT; i++)
    {
      state.getChannel(i).resetBeat();
    }
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