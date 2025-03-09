#include "Display.h"
#include "config.h"

Display::Display() {
    display = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
}

void Display::begin() {
    display->begin();
    display->setFont(u8g2_font_t0_11_tr);
}

void Display::update(const MetronomeState& state) {
    display->clearBuffer();
    
    // Draw 1px border around the whole screen
    display->drawFrame(0, 0, 128, 64);
    
    drawGlobalRow(state);
    drawGlobalProgress(state);
    
    // Add horizontal line after global section
    display->drawHLine(1, 13, 126);
    
    drawChannelBlock(state, 0, 14); // First channel at 14px
    
    // Add horizontal line between channel 1 and channel 2
    display->drawHLine(1, 37, 126);
    
    drawChannelBlock(state, 1, 38); // Second channel at 38px
    
    if (state.isRunning) {
        drawFlash(millis());
    }
    
    display->sendBuffer();
}

void Display::drawGlobalRow(const MetronomeState& state) {
    char buffer[32];
    
    // BPM display with selection frame
    sprintf(buffer, "%d BPM", state.bpm);
    if (state.isBpmSelected()) {
        display->drawFrame(1, 1, 45, 10); // Adjusted for border
        if (state.isEditing) {
            display->drawBox(1, 1, 45, 10);
            display->setDrawColor(0);
        }
    }
    display->drawStr(3, 9, buffer); // Adjusted for border
    display->setDrawColor(1);

    // Multiplier display
    sprintf(buffer, "x%s", state.getCurrentMultiplierName());
    if (state.isMultiplierSelected()) {
        display->drawFrame(49, 1, 30, 10); // Adjusted for border
        if (state.isEditing) {
            display->drawBox(49, 1, 30, 10);
            display->setDrawColor(0);
        }
    }
    display->drawStr(51, 9, buffer); // Adjusted for border
    display->setDrawColor(1);
    
    // Beat counter on the right
    uint32_t totalBeats = state.getTotalBeats();
    uint32_t currentBeat = (state.globalTick % totalBeats) + 1;
    sprintf(buffer, "%lu/%lu", currentBeat, totalBeats);
    display->drawStr(86, 9, buffer); // Adjusted for border
}

void Display::drawGlobalProgress(const MetronomeState& state) {
    if (!state.isRunning) return;
    
    float progress = float(state.globalTick % state.getTotalBeats()) / state.getTotalBeats();
    uint8_t width = uint8_t(progress * (SCREEN_WIDTH - 2)); // Adjusted for border
    display->drawBox(1, 11, width, 1); // Adjusted for border
}

void Display::drawChannelBlock(const MetronomeState& state, uint8_t channelIndex, uint8_t y) {
    const MetronomeChannel& channel = state.getChannel(channelIndex);
    char buffer[32];
    
    // Length display with blinking effect when active
    bool isActive = channel.isEnabled();
    bool shouldBlink = false;
    
    if (isActive && state.isRunning) {
        // Determine if we should blink based on beat state and timing
        BeatState beatState = channel.getBeatState();
        uint32_t currentTime = millis();
        uint32_t lastBeatTime = state.lastBeatTime;
        uint32_t blinkDuration = 0;
        
        switch(beatState) {
            case ACCENT:
                blinkDuration = ACCENT_PULSE_MS * 3;
                break;
            case WEAK:
                blinkDuration = SOLENOID_PULSE_MS * 3;
                break;
            default:
                blinkDuration = 0;
                break;
        }
        
        shouldBlink = (currentTime - lastBeatTime < blinkDuration);
    }
    
    // Length row
    sprintf(buffer, "%02d", channel.getBarLength());
    bool isLengthSelected = state.isLengthSelected(channelIndex);
    
    // Box for length
    uint8_t boxWidth = 20;
    uint8_t boxX = 1; // Adjusted for border
    
    if (isLengthSelected) {
        display->drawFrame(1, y, 120, 10); // Adjusted for border
        if (state.isEditing) {
            display->drawBox(1, y, 120, 10);
            display->setDrawColor(0);
        }
    }
    
    // Invert colors if we should blink
    if (shouldBlink) {
        display->drawBox(boxX, y, boxWidth, 10);
        display->setDrawColor(0); // White text on black
        display->drawStr(boxX + 2, y + 8, buffer);
        display->setDrawColor(1); // Back to normal
    } else {
        // Only draw the frame when blinking or selected
        if (isLengthSelected) {
            display->drawStr(boxX + 2, y + 8, buffer);
        } else {
            // Just draw the number without the frame
            display->drawStr(boxX + 2, y + 8, buffer);
        }
    }
    
    // Add pattern counter (current/total)
    uint16_t currentPattern = channel.getPattern();
    uint16_t maxPattern = channel.getMaxPattern();
    sprintf(buffer, "%u/%u", currentPattern, maxPattern);
    display->drawStr(85, y + 8, buffer);
    
    display->setDrawColor(1);
    
    // Pattern row - removed gap between rows
    uint8_t patternY = y + 10; // Directly below length row
    bool isPatternSelected = state.isPatternSelected(channelIndex);
    
    if (isPatternSelected) {
        display->drawFrame(1, patternY, 120, 10); // Adjusted for border
        if (state.isEditing) {
            display->drawBox(1, patternY, 120, 10);
            display->setDrawColor(0);
        }
    }
    
    drawBeatGrid(2, patternY + 1, channel, false);
    display->setDrawColor(1);
}

void Display::drawBeatGrid(uint8_t x, uint8_t y, const MetronomeChannel& ch, bool isEditing) {
    uint8_t cellWidth = 120 / ch.getBarLength();
    
    for (uint8_t i = 0; i < ch.getBarLength(); i++) {
        uint8_t cellX = x + (i * cellWidth);
        bool isCurrentBeat = (i == ch.getCurrentBeat());
        bool isBeatActive = ch.getPatternBit(i);
        
        if (isEditing && i == ch.getEditStep()) {
            display->drawFrame(cellX, y, cellWidth - 1, 8);
        }
        
        if (isBeatActive) {
            if (isCurrentBeat && ch.isEnabled()) {
                display->drawBox(cellX + 2, y + 2, cellWidth - 5, 4);
            } else {
                display->drawDisc(cellX + cellWidth / 2, y + 4, 2);
            }
        } else if (isCurrentBeat && ch.isEnabled()) {
            // Pulsing dot for silent active beat
            float pulse = (millis() % 500) / 500.0f; // 0.0 to 1.0 over 500ms
            uint8_t radius = 1 + pulse;
            display->drawDisc(cellX + cellWidth / 2, y + 4, radius);
        } else {
            display->drawPixel(cellX + cellWidth / 2, y + 4);
        }
    }
}

void Display::drawFlash(uint32_t currentTime) {
    static uint32_t lastFlashTime = 0;
    static const uint32_t FLASH_DURATION = 50;
    
    if (currentTime - lastFlashTime < FLASH_DURATION) {
        display->drawFrame(0, 0, 128, 64);
    }
}