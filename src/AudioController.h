#ifndef AUDIO_CONTROLLER_H
#define AUDIO_CONTROLLER_H

#include <Arduino.h>
#include <Ticker.h>
#include <driver/ledc.h>
#include <driver/dac.h>
#include "MetronomeState.h"
#include "config.h"

// Structure to hold sound state for each channel
struct ChannelSound {
  bool active = false;
  uint16_t frequency = 0;
  uint8_t volume = 0;
  uint32_t startTime = 0;
  uint16_t duration = 0;
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
  uint8_t toneVolume = 192;  // PWM duty cycle for tone component (0-255)
  
  // Random seed for noise generation
  uint32_t noiseSeed = 42;
  
  static AudioController *_instance;
  static void IRAM_ATTR endSoundCallback();
  static void mixerCallback();

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

  void init()
  {
    // Use the DAC for audio output
    dac_output_enable(DAC_CHANNEL_1); // GPIO25
    dac_output_voltage(DAC_CHANNEL_1, 0); // Start with no sound
    
    // Start the mixer timer to continuously mix and output audio
    // This runs at a higher frequency to generate smooth audio
    mixerTicker.attach_ms(AUDIO_MIXER_INTERVAL_MS, mixerCallback);
  }

  void processBeat(uint8_t channel, BeatState beatState)
  {
    if (channel >= MetronomeState::CHANNEL_COUNT) return;
    
    if (beatState == ACCENT || beatState == WEAK)
    {
      // Set the channel sound as active
      channelSounds[channel].active = true;
      
      // Use different volumes for accent vs normal beats
      channelSounds[channel].volume = (beatState == ACCENT) ? toneVolume : (toneVolume / 2);
      
      // Set the frequency for this channel
      channelSounds[channel].frequency = channelFrequencies[channel];
      
      // Record start time and duration
      channelSounds[channel].startTime = millis();
      channelSounds[channel].duration = soundDurationMs;
    }
  }

  void IRAM_ATTR handleEndSound()
  {
    // This is now handled by the mixer callback based on duration
  }
  
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
    if (channel < MetronomeState::CHANNEL_COUNT) {
      channelFrequencies[channel] = frequency;
    }
  }

  bool isSoundActive() const
  {
    for (uint8_t i = 0; i < MetronomeState::CHANNEL_COUNT; i++) {
      if (channelSounds[i].active) return true;
    }
    return false;
  }
};

#endif // AUDIO_CONTROLLER_H
