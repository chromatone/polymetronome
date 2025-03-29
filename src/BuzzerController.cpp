#include "BuzzerController.h"

BuzzerController *BuzzerController::_instance = nullptr;

void IRAM_ATTR BuzzerController::endSoundCallback()
{
  if (_instance)
  {
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    _instance->isPlaying = false;
  }
}

void BuzzerController::init()
{
  ledcSetup(0, SoundConfig::PWM_FREQ, SoundConfig::PWM_RES);
  ledcSetup(1, SoundConfig::PWM_FREQ, SoundConfig::PWM_RES);
  ledcAttachPin(buzzerPin1, 0);
  ledcAttachPin(buzzerPin2, 1);
}

void BuzzerController::playSound(uint8_t channel, const SoundParams &params)
{
  uint8_t pwmChannel = (channel == 0) ? 0 : 1;

  // First set volume to avoid clicks
  ledcWrite(pwmChannel, params.volume);
  delayMicroseconds(100);

  // Then set frequency
  ledcChangeFrequency(pwmChannel, params.frequency, SoundConfig::PWM_RES);

  isPlaying = true;
  soundTicker.once_ms(params.duration, endSoundCallback);
}

void BuzzerController::stopSound(uint8_t channel)
{
  uint8_t pwmChannel = (channel == 0) ? 0 : 1;
  ledcWrite(pwmChannel, 0);
}

void BuzzerController::processBeat(uint8_t channel, BeatState beatState)
{
  if (channel >= MetronomeState::CHANNEL_COUNT)
    return;

  soundTicker.detach(); // Cancel any pending sound end

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

void BuzzerController::update()
{
  // Not needed anymore as we use Ticker for timing
}