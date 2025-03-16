#ifndef AUDIO_CONTROLLER_H
#define AUDIO_CONTROLLER_H

#include <Arduino.h>
#include <Ticker.h>
#include <driver/ledc.h>
#include <driver/dac.h>
#include "MetronomeState.h"
#include "config.h"

// ADSR envelope structure
struct EnvelopeADSR {
  uint16_t attackTime = 5;    // ms
  uint16_t decayTime = 15;    // ms
  uint8_t sustainLevel = 70;  // percentage of peak
  uint16_t releaseTime = 30;  // ms
};

// Structure to hold sound state for each channel
struct ChannelSound
{
  bool active = false;
  uint16_t frequency = 0;
  uint8_t volume = 0;
  uint32_t startTime = 0;
  uint16_t duration = 0;
  uint8_t waveformType = 0;   // 0=sine, 1=triangle, 2=square, 3=noise
  EnvelopeADSR envelope;
};

enum WaveformType {
  SINE_WAVE = 0,
  TRIANGLE_WAVE = 1,
  SQUARE_WAVE = 2,
  NOISE = 3,
  FM_SINE = 4
};

class AudioController
{
private:
  uint8_t audioPin;
  uint16_t soundDurationMs;
  Ticker soundTicker;
  Ticker mixerTicker;

  // Sound parameters for each channel
  ChannelSound channelSounds[MetronomeState::CHANNEL_COUNT];

  // Base frequencies for each channel
  uint16_t channelFrequencies[MetronomeState::CHANNEL_COUNT] = {220, 330};

  // Noise parameters
  uint8_t noiseVolume = 64; // PWM duty cycle for noise component (0-255)
  uint8_t toneVolume = 192; // PWM duty cycle for tone component (0-255)

  // FM synthesis parameters
  float modIndex = 2.0f;       // Modulation index for FM synthesis
  float modFreqRatio = 2.0f;   // Modulation frequency ratio

  // Dithering variables
  float dither_prev = 0.0f;
  
  // Lookup table for sine wave
  static const uint16_t SINE_TABLE_SIZE = 256;
  uint8_t sineTable[SINE_TABLE_SIZE];

  // Random seed for noise generation
  uint32_t noiseSeed = 42;

  static AudioController *_instance;
  static void IRAM_ATTR endSoundCallback();
  static void mixerCallback();

  // Generate sample based on waveform type and phase (0.0-1.0)
  int16_t generateSample(uint8_t waveformType, float phase, uint8_t channel);
  
  // Apply ADSR envelope to a sample
  float applyEnvelope(ChannelSound &sound, uint32_t currentTime);
  
  // Apply triangular probability density function (TPDF) dithering
  int16_t applyDithering(float sample);

public:
  AudioController(uint8_t pin, uint16_t durationMs = SOUND_DURATION_MS)
      : audioPin(pin), soundDurationMs(durationMs)
  {
    _instance = this;

    // Initialize channel frequencies
    channelFrequencies[0] = AUDIO_FREQ_CH1;
    channelFrequencies[1] = AUDIO_FREQ_CH2;
  }

  ~AudioController()
  {
    if (_instance == this)
    {
      _instance = nullptr;
    }
    mixerTicker.detach();
    soundTicker.detach();
  }

  void init();  // Declaration only

  void processBeat(uint8_t channel, BeatState beatState);

  void IRAM_ATTR handleEndSound();

  // Declaration only - implementation in CPP file to avoid IRAM issues
  void handleMixer();

  void setSoundDuration(uint16_t durationMs)
  {
    soundDurationMs = durationMs;
  }

  void setToneVolume(uint8_t vol)
  {
    toneVolume = constrain(vol, 0, 255);
  }

  void setNoiseVolume(uint8_t vol)
  {
    noiseVolume = constrain(vol, 0, 255);
  }

  void setChannelFrequency(uint8_t channel, uint16_t frequency)
  {
    if (channel < MetronomeState::CHANNEL_COUNT)
    {
      channelFrequencies[channel] = frequency;
    }
  }
  
  void setWaveformType(uint8_t channel, uint8_t type) {
    if (channel < MetronomeState::CHANNEL_COUNT) {
      channelSounds[channel].waveformType = type;
    }
  }
  
  void setEnvelopeParams(uint8_t channel, uint16_t attack, uint16_t decay, 
                         uint8_t sustain, uint16_t release) {
    if (channel < MetronomeState::CHANNEL_COUNT) {
      channelSounds[channel].envelope.attackTime = attack;
      channelSounds[channel].envelope.decayTime = decay;
      channelSounds[channel].envelope.sustainLevel = sustain;
      channelSounds[channel].envelope.releaseTime = release;
    }
  }
  
  void setFMParams(float index, float freqRatio) {
    modIndex = index;
    modFreqRatio = freqRatio;
  }

  bool isSoundActive() const
  {
    for (uint8_t i = 0; i < MetronomeState::CHANNEL_COUNT; i++)
    {
      if (channelSounds[i].active)
        return true;
    }
    return false;
  }
};

#endif // AUDIO_CONTROLLER_H
