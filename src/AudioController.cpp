#include "AudioController.h"

// Initialize static member
AudioController* AudioController::_instance = nullptr;

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

// Implementation of handleMixer moved from header to avoid IRAM issues
void AudioController::handleMixer()
{
  // Check if any sounds have expired
  uint32_t currentTime = millis();
  bool anyActive = false;
  int16_t mixedSample = 0;
  
  // Process each channel
  for (uint8_t i = 0; i < MetronomeState::CHANNEL_COUNT; i++) {
    // Check if sound has expired
    if (channelSounds[i].active) {
      if (currentTime - channelSounds[i].startTime >= channelSounds[i].duration) {
        channelSounds[i].active = false;
      } else {
        // Sound is still active
        anyActive = true;
        
        // Simple tone generation - square wave
        uint32_t tonePhase = (currentTime * channelSounds[i].frequency / 500) % 2;
        if (tonePhase == 0) {
          mixedSample += channelSounds[i].volume;
        } else {
          mixedSample -= channelSounds[i].volume;
        }
      }
    }
  }
  
  // Add noise component if any channel is active
  if (anyActive) {
    // Simple pseudo-random noise generation
    noiseSeed = noiseSeed * 1103515245 + 12345;
    int8_t noise = (noiseSeed >> 16) & 0xFF;
    mixedSample += (noise * noiseVolume) / 255;
    
    // Scale and constrain the output
    mixedSample = constrain(mixedSample + 128, 0, 255);
    
    // Output to DAC
    dac_output_voltage(DAC_CHANNEL_1, mixedSample);
  } else {
    // No active sounds, turn off DAC
    dac_output_voltage(DAC_CHANNEL_1, 0);
  }
}
