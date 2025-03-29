#include "BuzzerController.h"

void BuzzerController::init()
{
  // Configure PWM channels for both buzzers
  ledcSetup(0, 40000, 8); // Channel 0 for first buzzer
  ledcSetup(1, 40000, 8); // Channel 1 for second buzzer

  ledcAttachPin(buzzerPin1, 0);
  ledcAttachPin(buzzerPin2, 1);

  // Initialize both with no sound
  ledcWrite(0, 0);
  ledcWrite(1, 0);
}

void BuzzerController::playSound(uint8_t channel, const SoundParams &params)
{
  uint8_t pwmChannel = (channel == 0) ? 0 : 1;
  SoundState &state = (channel == 0) ? channel1State : channel2State;

  ledcChangeFrequency(pwmChannel, params.frequency, 8);
  ledcWrite(pwmChannel, params.volume);

  state.startTime = millis();
  state.soundDuration = params.duration;
  state.isPlaying = true;
}

void BuzzerController::stopSound(uint8_t channel)
{
  ledcWrite(channel, 0);
  if (channel == 0)
  {
    channel1State.isPlaying = false;
  }
  else
  {
    channel2State.isPlaying = false;
  }
}

void BuzzerController::update()
{
  // Update channel 1
  if (channel1State.isPlaying &&
      (millis() - channel1State.startTime >= channel1State.soundDuration))
  {
    stopSound(0);
  }

  // Update channel 2
  if (channel2State.isPlaying &&
      (millis() - channel2State.startTime >= channel2State.soundDuration))
  {
    stopSound(1);
  }
}

void BuzzerController::processBeat(uint8_t channel, BeatState beatState)
{
  if (channel >= MetronomeState::CHANNEL_COUNT)
    return;

  switch (beatState)
  {
  case ACCENT:
    playSound(channel, channel == 0 ? ch1Strong : ch2Strong);
    break;

  case WEAK:
    playSound(channel, channel == 0 ? ch1Weak : ch2Weak);
    break;

  case SILENT:
    stopSound(channel);
    break;
  }
}
