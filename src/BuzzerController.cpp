#include "BuzzerController.h"

// Define patterns here
const uint8_t PROGMEM BuzzerController::strongPattern[SoundConfig::PATTERN_LENGTH] = {255, 192, 128, 96, 64, 48, 32, 16};
const uint8_t PROGMEM BuzzerController::weakPattern[SoundConfig::PATTERN_LENGTH] = {192, 128, 96, 64, 48, 32, 16, 8};

BuzzerController *BuzzerController::_instance = nullptr;

void IRAM_ATTR BuzzerController::endSoundCallback()
{
  if (_instance)
  {
    _instance->channel1State.currentVolume = 0;
    _instance->channel2State.currentVolume = 0;
    _instance->silencePins();
    _instance->isPlaying = false;
  }
}

void BuzzerController::init()
{
  setPinMode(MODE_INPUT);
  ledcSetup(0, SoundConfig::PWM_FREQ, SoundConfig::PWM_RES);
  ledcSetup(1, SoundConfig::PWM_FREQ, SoundConfig::PWM_RES);
  delay(1);
  setPinMode(MODE_DIGITAL);
}

void BuzzerController::setPinMode(PinMode mode)
{
  if (mode == currentMode)
    return;

  switch (mode)
  {
  case MODE_INPUT:
    pinMode(buzzerPin1, INPUT);
    pinMode(buzzerPin2, INPUT);
    break;

  case MODE_DIGITAL:
    pinMode(buzzerPin1, OUTPUT);
    pinMode(buzzerPin2, OUTPUT);
    digitalWrite(buzzerPin1, LOW);
    digitalWrite(buzzerPin2, LOW);
    break;

  case MODE_PWM:
    setPinMode(MODE_DIGITAL); // Ensure clean transition
    delay(1);
    ledcAttachPin(buzzerPin1, 0);
    ledcAttachPin(buzzerPin2, 1);
    break;
  }

  currentMode = mode;
}

void BuzzerController::silencePins()
{
  if (currentMode == MODE_PWM)
  {
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    ledcDetachPin(buzzerPin1);
    ledcDetachPin(buzzerPin2);
  }
  setPinMode(MODE_DIGITAL);
}

void BuzzerController::attachPins()
{
  if (!pinsAttached)
  {
    ledcAttachPin(buzzerPin1, 0);
    ledcAttachPin(buzzerPin2, 1);
    pinsAttached = true;
  }
}

void BuzzerController::detachPins()
{
  if (pinsAttached)
  {
    ledcDetachPin(buzzerPin1);
    ledcDetachPin(buzzerPin2);
    pinsAttached = false;
  }
}

void BuzzerController::rampVolume(uint8_t channel, uint8_t targetVolume)
{
  uint8_t pwmChannel = (channel == 0) ? 0 : 1;
  SoundState &state = (channel == 0) ? channel1State : channel2State;

  int8_t step = (targetVolume - state.currentVolume) / SoundConfig::RAMP_STEPS;

  for (uint8_t i = 0; i < SoundConfig::RAMP_STEPS; i++)
  {
    state.currentVolume += step;
    ledcWrite(pwmChannel, state.currentVolume);
    delay(SoundConfig::RAMP_DELAY);
  }

  // Ensure we reach exactly the target volume
  state.currentVolume = targetVolume;
  ledcWrite(pwmChannel, targetVolume);
}

void BuzzerController::performFrequencySweep(uint8_t channel, uint16_t targetFreq)
{
  uint8_t pwmChannel = (channel == 0) ? 0 : 1;
  uint16_t startFreq = targetFreq * SoundConfig::SWEEP_MULTIPLIER;
  uint16_t freqStep = (startFreq - targetFreq) / SoundConfig::SWEEP_STEPS;

  uint16_t currentFreq = startFreq;
  for (uint8_t i = 0; i < SoundConfig::SWEEP_STEPS; i++)
  {
    ledcChangeFrequency(pwmChannel, currentFreq, SoundConfig::PWM_RES);
    currentFreq -= freqStep;
    delay(SoundConfig::SWEEP_DURATION / SoundConfig::SWEEP_STEPS);
  }

  // Ensure we end at exactly the target frequency
  ledcChangeFrequency(pwmChannel, targetFreq, SoundConfig::PWM_RES);
}

void BuzzerController::applyPWMPattern(uint8_t channel, const uint8_t *pattern, uint8_t volume)
{
  uint8_t pwmChannel = (channel == 0) ? 0 : 1;

  // Apply pattern multiple times during attack phase
  for (uint8_t repeat = 0; repeat < SoundConfig::PATTERN_REPEAT; repeat++)
  {
    for (uint8_t i = 0; i < SoundConfig::PATTERN_LENGTH; i++)
    {
      uint8_t patternValue = pgm_read_byte(&pattern[i]);
      // Scale pattern value by volume
      uint16_t scaledValue = (uint16_t)patternValue * volume / 255;
      ledcWrite(pwmChannel, scaledValue);
      delay(1); // 1ms per step
    }
  }
  // Set final volume
  ledcWrite(pwmChannel, volume);
}

void BuzzerController::applyNoiseMix(uint8_t channel, uint8_t volume, uint8_t noiseRatio)
{
  if (noiseRatio == 0)
    return;

  uint8_t pwmChannel = (channel == 0) ? 0 : 1;
  uint32_t startTime = millis();
  uint32_t duration = SoundConfig::CH1_DUR; // Use standard duration

  while (millis() - startTime < duration)
  {
    // Generate and scale noise
    uint8_t noise = (esp_random() & 0xFF);
    uint16_t mixed = ((volume * (255 - noiseRatio)) + (noise * noiseRatio)) / 255;
    ledcWrite(pwmChannel, mixed);
    delay(SoundConfig::NOISE_UPDATE_RATE);
  }
}

void BuzzerController::playSound(uint8_t channel, const SoundParams &params)
{
  setPinMode(MODE_PWM);
  uint8_t pwmChannel = (channel == 0) ? 0 : 1;

  if (params.useFrequencySweep)
  {
    performFrequencySweep(channel, params.frequency);
  }

  ledcChangeFrequency(pwmChannel, params.frequency, SoundConfig::PWM_RES);

  // Apply PWM pattern for attack phase
  applyPWMPattern(channel, params.pwmPattern, params.volume);

  // Apply noise mix if ratio > 0
  if (params.noiseRatio > 0)
  {
    applyNoiseMix(channel, params.volume, params.noiseRatio);
  }

  isPlaying = true;
  soundTicker.once_ms(params.duration, endSoundCallback);
}

void BuzzerController::stopSound(uint8_t channel)
{
  if (channel >= MetronomeState::CHANNEL_COUNT)
    return;

  if (!channel1State.isPlaying && !channel2State.isPlaying)
  {
    silencePins();
  }
  else
  {
    uint8_t pwmChannel = (channel == 0) ? 0 : 1;
    ledcWrite(pwmChannel, 0);
  }
}

uint8_t BuzzerController::getAdjustedVolume(uint8_t channel, uint8_t baseVolume) const
{
  const MetronomeState &state = MetronomeState::getInstance();
  return (uint16_t)baseVolume * state.getChannel(channel).getVolume() / 255;
}

void BuzzerController::processBeat(uint8_t channel, BeatState beatState)
{
  if (channel >= MetronomeState::CHANNEL_COUNT)
    return;

  soundTicker.detach();

  const MetronomeChannel &ch = MetronomeState::getInstance().getChannel(channel);

  switch (beatState)
  {
  case ACCENT:
  {
    SoundParams params = (channel == 0) ? ch1Strong : ch2Strong;
    params.volume = ch.getEffectiveStrongVolume();
    playSound(channel, params);
  }
  break;
  case WEAK:
  {
    SoundParams params = (channel == 0) ? ch1Weak : ch2Weak;
    params.volume = ch.getEffectiveWeakVolume();
    playSound(channel, params);
  }
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