#pragma once

// Pin definitions
#define ENCODER_A 17
#define ENCODER_B 18
#define ENCODER_BTN 16
#define BTN_START 4
#define BTN_STOP 19
#define SOLENOID_PIN 14
#define SOLENOID_PIN2 32
#define DAC_PIN 25     // ESP32 DAC1 pin (GPIO25)
#define BUZZER_PIN1 26 // ESP32 GPIO26 for PWM buzzer output (Channel 1)
#define BUZZER_PIN2 15 // ESP32 GPIO12 for PWM buzzer output (Channel 2)

// Display I2C pins
#define DISPLAY_SDA 21
#define DISPLAY_SCL 22

// Timing constants
#define MIN_GLOBAL_BPM 10
#define MAX_GLOBAL_BPM 300
#define DEFAULT_BPM 120
#define MAX_BEATS 16
#define SOLENOID_PULSE_MS 5
#define ACCENT_PULSE_MS 7
#define SOUND_DURATION_MS 25        // Duration of sound on each beat (in ms)
#define LONG_PRESS_DURATION_MS 1000 // Duration for long press in milliseconds

// Audio settings
#define AUDIO_FREQ_CH1 440        // Frequency for channel 1 in Hz
#define AUDIO_FREQ_CH2 880        // Frequency for channel 2 in Hz
#define AUDIO_MIXER_INTERVAL_MS 2 // Interval for audio mixer in milliseconds

// Multiplier values (in quarters)
#define MULTIPLIER_COUNT 4
#define MULTIPLIERS {1.0, 2.0, 4.0, 8.0}
#define MULTIPLIER_NAMES {"1", "2", "4", "8"}

// Display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// UI Constants
#define FLASH_DURATION_MS 50

// Configuration storage constants
#define CONFIG_VERSION 1
#define CONFIG_MAGIC_MARKER 0xCBEF // Magic bytes to verify config integrity

// Fixed number of channels (for now)
#define FIXED_CHANNEL_COUNT 2

// LED strip configuration
#define LED_BRIGHTNESS 50
#define LED_FLASH_DURATION_FRACTION 0.1f // Flash duration as fraction of beat
#define LED_BEAT_DURATION_MS 100
