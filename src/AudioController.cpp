#include "AudioController.h"
#include <math.h>

// Initialize static member
AudioController *AudioController::_instance = nullptr;

// Static callback function that will be called when the sound should end
void IRAM_ATTR AudioController::endSoundCallback()
{
  if (_instance)
  {
    _instance->handleEndSound();
  }
}

// Static callback function for the audio mixer
void AudioController::mixerCallback()
{
  if (_instance)
  {
    _instance->handleMixer();
  }
}

void AudioController::init()
{
  // Use the DAC for audio output
  dac_output_enable(DAC_CHANNEL_1);     // GPIO25
  dac_output_voltage(DAC_CHANNEL_1, 0); // Start with no sound

  // Initialize sine table with improved resolution
  for (int i = 0; i < SINE_TABLE_SIZE; i++)
  {
    // Generate sine wave values scaled to 0-255 range with slight bias toward positive values
    sineTable[i] = (uint8_t)(135.0f + 120.0f * sin(2.0f * PI * i / SINE_TABLE_SIZE));
  }

  // Initialize default ADSR settings for each channel with distinct characteristics
  for (uint8_t i = 0; i < MetronomeState::CHANNEL_COUNT; i++)
  {
    if (i == 0)
    { // Main beat channel - sharper, more prominent
      channelSounds[i].waveformType = FM_SINE;
      channelSounds[i].envelope.attackTime = 1;    // Very fast attack
      channelSounds[i].envelope.decayTime = 25;    // Longer decay for body
      channelSounds[i].envelope.sustainLevel = 50; // Medium sustain
      channelSounds[i].envelope.releaseTime = 35;  // Smooth release
    }
    else
    { // Secondary beat channel - softer, more tonal
      channelSounds[i].waveformType = SINE_WAVE;
      channelSounds[i].envelope.attackTime = 2;    // Slightly slower attack
      channelSounds[i].envelope.decayTime = 20;    // Moderate decay
      channelSounds[i].envelope.sustainLevel = 40; // Lower sustain
      channelSounds[i].envelope.releaseTime = 25;  // Shorter release
    }
  }

  // Start the mixer timer to continuously mix and output audio
  // This runs at a higher frequency to generate smooth audio
  mixerTicker.attach_ms(AUDIO_MIXER_INTERVAL_MS, mixerCallback);
}

// Generate a sample value based on waveform type and phase
int16_t AudioController::generateSample(uint8_t waveformType, float phase, uint8_t channel)
{
  switch (waveformType)
  {
  case SINE_WAVE:
  {
    // Pure sine wave with slight enhancement
    uint16_t index = (uint16_t)(phase * SINE_TABLE_SIZE) % SINE_TABLE_SIZE;
    return (int16_t)sineTable[index] - 128;
  }

  case TRIANGLE_WAVE:
  {
    // Enhanced triangle wave with slight curve
    float value = phase * 2.0f;
    if (value > 1.0f)
    {
      value = 2.0f - value;
    }
    // Add slight exponential curve for more "snap"
    value = powf(value, 0.7f);
    return (int16_t)((value - 0.5f) * 255.0f);
  }

  case FM_SINE:
  {
    // Enhanced FM synthesis with dynamic modulation
    float modFreq = channelSounds[channel].frequency * modFreqRatio;

    // Dynamic modulation index based on the envelope phase
    float dynamicModIndex = modIndex * (1.0f - phase * 0.5f); // Reduces modulation over time

    // Carrier and modulator phases
    float modPhase = fmod(phase * modFreq / channelSounds[channel].frequency, 1.0f);
    float modValue = sin(2.0f * PI * modPhase);

    // Apply modulation with a slight phase offset for richer harmonics
    float carrierPhase = fmod(phase + dynamicModIndex * modValue / (2.0f * PI) + 0.25f, 1.0f);
    float carrierValue = sin(2.0f * PI * carrierPhase);

    // Add a touch of the modulator signal for more "click"
    return (int16_t)((carrierValue * 0.9f + modValue * 0.1f) * 127.0f);
  }

  case NOISE:
  {
    // Improved noise with filtered characteristics
    noiseSeed = noiseSeed * 1103515245 + 12345;
    float noise = (float)((noiseSeed >> 16) & 0xFF) / 128.0f - 1.0f;

    // Filter the noise a bit and add some "color"
    float filtered = noise * 0.7f + dither_prev * 0.3f;
    dither_prev = filtered;

    return (int16_t)(filtered * 96.0f); // Reduced amplitude for noise
  }

  default:
    return 0;
  }
}

// Apply ADSR envelope with improved curve shaping
float AudioController::applyEnvelope(ChannelSound &sound, uint32_t currentTime)
{
  uint32_t elapsedTime = currentTime - sound.startTime;
  float amplitude = 0.0f;

  if (elapsedTime < sound.envelope.attackTime)
  {
    // Non-linear attack for better "snap"
    float progress = (float)elapsedTime / sound.envelope.attackTime;
    amplitude = powf(progress, 0.5f); // Faster initial attack
  }
  else if (elapsedTime < (sound.envelope.attackTime + sound.envelope.decayTime))
  {
    float decayProgress = (float)(elapsedTime - sound.envelope.attackTime) / sound.envelope.decayTime;
    // Exponential decay curve
    amplitude = 1.0f - powf(decayProgress, 0.7f) * (1.0f - sound.envelope.sustainLevel / 100.0f);
  }
  else if (elapsedTime < sound.duration - sound.envelope.releaseTime)
  {
    amplitude = sound.envelope.sustainLevel / 100.0f;
  }
  else if (elapsedTime < sound.duration)
  {
    float releaseProgress = (float)(elapsedTime - (sound.duration - sound.envelope.releaseTime)) / sound.envelope.releaseTime;
    // Smooth release curve
    amplitude = (sound.envelope.sustainLevel / 100.0f) * (1.0f - powf(releaseProgress, 1.5f));
  }

  return amplitude;
}

// Improved dithering with noise shaping
int16_t AudioController::applyDithering(float sample)
{
  // TPDF dithering with noise shaping
  float rand1 = (float)random(1000) / 1000.0f;
  float rand2 = (float)random(1000) / 1000.0f;
  float dither = (rand1 - rand2) / 512.0f; // Reduced dither amplitude

  // Apply noise shaping
  float shaped = sample + dither - dither_prev * 0.5f;
  int16_t quantized = (int16_t)shaped;

  // Store error for next sample
  dither_prev = shaped - quantized;

  return quantized;
}

// Implementation of handleMixer moved from header to avoid IRAM issues
void AudioController::handleMixer()
{
  uint32_t currentTime = millis();
  bool anyActive = false;
  float mixedSample = 0.0f;

  // Process each channel
  for (uint8_t i = 0; i < MetronomeState::CHANNEL_COUNT; i++)
  {
    if (channelSounds[i].active)
    {
      if (currentTime - channelSounds[i].startTime >= channelSounds[i].duration)
      {
        channelSounds[i].active = false;
      }
      else
      {
        anyActive = true;

        float timeSec = (float)(currentTime - channelSounds[i].startTime) / 1000.0f;
        float phase = fmod(timeSec * channelSounds[i].frequency, 1.0f);

        // Generate and process the sample
        int16_t rawSample = generateSample(channelSounds[i].waveformType, phase, i);
        float envelopeValue = applyEnvelope(channelSounds[i], currentTime);

        // Apply volume with slight compression
        float channelSample = (float)rawSample * envelopeValue;
        if (channelSample > 0)
        {
          channelSample = powf(channelSample / 127.0f, 0.8f) * 127.0f;
        }
        else
        {
          channelSample = -powf(-channelSample / 127.0f, 0.8f) * 127.0f;
        }

        channelSample *= channelSounds[i].volume / 255.0f;

        // Mix into final output
        mixedSample += channelSample;
      }
    }
  }

  // Soft clipping on the final mix
  if (mixedSample > 127.0f)
  {
    mixedSample = 127.0f * (2.0f / (1.0f + exp(-2.0f * (mixedSample - 127.0f) / 127.0f)));
  }
  else if (mixedSample < -128.0f)
  {
    mixedSample = -128.0f * (2.0f / (1.0f + exp(2.0f * (mixedSample + 128.0f) / 128.0f)));
  }

  // Apply dithering and output
  int16_t ditheredSample = applyDithering(mixedSample);
  uint8_t outputValue = constrain(ditheredSample + 128, 0, 255);

  if (anyActive)
  {
    dac_output_voltage(DAC_CHANNEL_1, outputValue);
  }
  else
  {
    dac_output_voltage(DAC_CHANNEL_1, 0);
  }
}

void AudioController::processBeat(uint8_t channel, BeatState beatState)
{
  if (channel >= MetronomeState::CHANNEL_COUNT)
    return;

  if (beatState == ACCENT || beatState == WEAK)
  {
    channelSounds[channel].active = true;

    if (beatState == ACCENT)
    {
      // Accent beats - more prominent, with FM synthesis
      channelSounds[channel].waveformType = FM_SINE;
      channelSounds[channel].volume = toneVolume;
      channelSounds[channel].frequency = channelFrequencies[channel];

      // Punchy envelope for accents
      channelSounds[channel].envelope.attackTime = 1;
      channelSounds[channel].envelope.decayTime = 30;
      channelSounds[channel].envelope.sustainLevel = 60;
      channelSounds[channel].envelope.releaseTime = 40;

      // Higher modulation for accents
      modIndex = 3.0f;
      modFreqRatio = 2.5f;
    }
    else
    {
      // Weak beats - cleaner, simpler sound
      channelSounds[channel].volume = toneVolume * 0.6f;

      if (channel == 0)
      {
        // First channel weak beats - clean sine with slight FM
        channelSounds[channel].waveformType = FM_SINE;
        channelSounds[channel].frequency = channelFrequencies[channel] * 1.5f;
        modIndex = 1.2f;
        modFreqRatio = 1.5f;
      }
      else
      {
        // Second channel weak beats - pure sine
        channelSounds[channel].waveformType = SINE_WAVE;
        channelSounds[channel].frequency = channelFrequencies[channel];
      }

      // Softer envelope for weak beats
      channelSounds[channel].envelope.attackTime = 2;
      channelSounds[channel].envelope.decayTime = 15;
      channelSounds[channel].envelope.sustainLevel = 40;
      channelSounds[channel].envelope.releaseTime = 25;
    }

    channelSounds[channel].startTime = millis();
    channelSounds[channel].duration = (beatState == ACCENT) ? (soundDurationMs * 1.5) : soundDurationMs;
  }
}

void IRAM_ATTR AudioController::handleEndSound()
{
  // This method is maintained for compatibility
  // Sound termination is now handled in the mixer using duration and envelope
}
