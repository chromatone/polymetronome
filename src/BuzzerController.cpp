#include "BuzzerController.h"

void BuzzerController::init()
{
  // Configure LED PWM timer with higher frequency for audio
  ledcSetup(0, 40000, 8); // Channel 0, 40kHz base freq for better audio, 8-bit resolution
  ledcAttachPin(buzzerPin, 0);

  // Initialize with no sound
  ledcWrite(0, 0);
}

void BuzzerController::playSound(const SoundParams &params)
{
  // Set frequency - use a much higher base frequency
  ledcChangeFrequency(0, params.frequency, 8);
  // Set volume through PWM duty cycle
  ledcWrite(0, params.volume);

  // Use millis() for non-blocking duration timing
  startTime = millis();
  soundDuration = params.duration;
  isPlaying = true;
}

void BuzzerController::update()
{
  if (isPlaying && (millis() - startTime >= soundDuration))
  {
    stopSound();
    isPlaying = false;
  }
}

void BuzzerController::stopSound()
{
  ledcWrite(0, 0);
}

void BuzzerController::processBeat(uint8_t channel, BeatState beatState)
{
  if (channel >= MetronomeState::CHANNEL_COUNT)
    return;

  switch (beatState)
  {
  case ACCENT:
    if (channel == 0)
    {
      playSound(ch1Strong);
    }
    else
    {
      playSound(ch2Strong);
    }
    break;

  case WEAK:
    if (channel == 0)
    {
      playSound(ch1Weak);
    }
    else
    {
      playSound(ch2Weak);
    }
    break;

  case SILENT:
    stopSound();
    break;
  }
}
