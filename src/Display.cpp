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
    
    drawGlobalRow(state);
    drawGlobalProgress(state);
    
    drawChannelBlock(state, 0, 13); // First channel at 13px
    drawChannelBlock(state, 1, 35); // Second channel at 35px (13 + 22)
    
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
        display->drawFrame(0, 1, 45, 10); // Start at y=1 for padding
        if (state.isEditing) {
            display->drawBox(0, 1, 45, 10);
            display->setDrawColor(0);
        }
    }
    display->drawStr(2, 9, buffer); // Move text down 1px
    display->setDrawColor(1);

    // Multiplier display
    sprintf(buffer, "x%s", state.getCurrentMultiplierName());
    if (state.isMultiplierSelected()) {
        display->drawFrame(48, 1, 30, 10); // Start at y=1 for padding
        if (state.isEditing) {
            display->drawBox(48, 1, 30, 10);
            display->setDrawColor(0);
        }
    }
    display->drawStr(50, 9, buffer); // Move text down 1px
    display->setDrawColor(1);
    
    // Beat counter on the right
    uint32_t totalBeats = state.getTotalBeats();
    uint32_t currentBeat = (state.globalTick % totalBeats) + 1;
    sprintf(buffer, "%lu/%lu", currentBeat, totalBeats);
    display->drawStr(85, 9, buffer); // Move text down 1px
}

void Display::drawGlobalProgress(const MetronomeState& state) {
    if (!state.isRunning) return;
    
    float progress = float(state.globalTick % state.getTotalBeats()) / state.getTotalBeats();
    uint8_t width = uint8_t(progress * SCREEN_WIDTH);
    display->drawBox(0, 11, width, 1); // Single pixel progress bar at y=11
}

void Display::drawChannelBlock(const MetronomeState& state, uint8_t channelIndex, uint8_t y) {
    const MetronomeChannel& channel = state.getChannel(channelIndex);
    char buffer[32];
    
    // Length row
    sprintf(buffer, "%02d", channel.getBarLength());
    bool isLengthSelected = state.isLengthSelected(channelIndex);
    
    if (isLengthSelected) {
        display->drawFrame(0, y, 120, 10); // Reduced height to 10px
        if (state.isEditing) {
            display->drawBox(0, y, 120, 10);
            display->setDrawColor(0);
        }
    }
    
    drawClickIndicator(2, y+1, channel.getBeatState(), channel.isEnabled());
    display->drawStr(20, y+8, buffer);
    display->setDrawColor(1);
    
    // Pattern row - removed gap between rows
    uint8_t patternY = y + 10; // Directly below length row
    bool isPatternSelected = state.isPatternSelected(channelIndex);
    
    if (isPatternSelected) {
        display->drawFrame(0, patternY, 120, 10);
        if (state.isEditing) {
            display->drawBox(0, patternY, 120, 10);
            display->setDrawColor(0);
        }
    }
    
    drawBeatGrid(2, patternY+1, channel, false);
    display->setDrawColor(1);
}

void Display::drawClickIndicator(uint8_t x, uint8_t y, BeatState state, bool isActive) {
    if (isActive) {
        display->drawFrame(x, y, 16, 10);
    }
    
    switch(state) {
        case ACCENT:
            display->drawBox(x+4, y+2, 8, 6);
            break;
        case WEAK:
            display->drawFrame(x+4, y+2, 8, 6);
            break;
        case SILENT:
            display->drawPixel(x+8, y+5);
            break;
    }
}

void Display::drawBeatGrid(uint8_t x, uint8_t y, const MetronomeChannel& ch, bool isEditing) {
    uint8_t cellWidth = 120 / ch.getBarLength();
    
    for (uint8_t i = 0; i < ch.getBarLength(); i++) {
        uint8_t cellX = x + (i * cellWidth);
        bool isCurrentBeat = (i == ch.getCurrentBeat());
        bool isBeatActive = ch.getPatternBit(i);
        
        if (isEditing && i == ch.getEditStep()) {
            display->drawFrame(cellX, y, cellWidth-1, 8);
        }
        
        if (isBeatActive) {
            if (isCurrentBeat && ch.isEnabled()) {
                display->drawBox(cellX+2, y+2, cellWidth-5, 4);
            } else {
                display->drawDisc(cellX + cellWidth/2, y+4, 2);
            }
        } else if (isCurrentBeat && ch.isEnabled()) {
            // Pulsing dot for silent active beat
            float pulse = (millis() % 500) / 500.0f; // 0.0 to 1.0 over 500ms
            uint8_t radius = 1 + pulse;
            display->drawDisc(cellX + cellWidth/2, y+4, radius);
        } else {
            display->drawPixel(cellX + cellWidth/2, y+4);
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
