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
    
    // Global status row
    drawGlobalRow(state);
    
    // Global progress bar at y=12
    drawProgressBar(12, state.globalTick / 60000.0f);
    
    // Channel blocks
    drawChannelBlock(state, 0, 16); // First channel
    drawChannelBlock(state, 1, 40); // Second channel
    
    // Flash border on beats if running
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
        display->drawFrame(0, 0, 45, 11);
        if (state.isEditing) {
            display->drawBox(0, 0, 45, 11);
            display->setDrawColor(0);
        }
    }
    display->drawStr(2, 8, buffer);
    display->setDrawColor(1);
    
    // Beat counter on the right
    uint32_t totalBeats = state.getChannel(0).getBarLength() * state.getChannel(1).getBarLength();
    sprintf(buffer, "%lu/%lu", (state.globalTick / 60000) % totalBeats + 1, totalBeats);
    display->drawStr(85, 8, buffer);
}

void Display::drawProgressBar(uint8_t y, float progress) {
    uint8_t width = progress * 124;
    display->drawFrame(2, y, 124, 2);
    if (width > 0) {
        display->drawBox(2, y, width, 2);
    }
}

void Display::drawChannelBlock(const MetronomeState& state, uint8_t channelIndex, uint8_t y) {
    const MetronomeChannel& channel = state.getChannel(channelIndex);
    char buffer[32];
    
    // Length row (e.g., "04")
    sprintf(buffer, "%02d", channel.getBarLength());
    bool isLengthSelected = state.isLengthSelected(channelIndex);
    
    if (isLengthSelected) {
        display->drawFrame(0, y, 120, 12);
        if (state.isEditing) {
            display->drawBox(0, y, 120, 12);
            display->setDrawColor(0);
        }
    }
    
    // Click indicator and length
    drawClickIndicator(2, y+2, channel.getBeatState(), channel.isEnabled());
    display->drawStr(20, y+10, buffer);
    display->setDrawColor(1);
    
    // Progress bar
    drawProgressBar(y+14, channel.getProgress(millis(), state.bpm));
    
    // Pattern row
    uint8_t patternY = y + 16;
    bool isPatternSelected = state.isPatternSelected(channelIndex);
    
    if (isPatternSelected) {
        display->drawFrame(0, patternY, 120, 12);
        if (state.isEditing) {
            display->drawBox(0, patternY, 120, 12);
            display->setDrawColor(0);
        }
    }
    
    drawBeatGrid(2, patternY+2, channel, false);
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
        bool isBeatActive = ch.getPatternBit(i); // Fixed: use getPatternBit directly
        
        if (isEditing && i == ch.getEditStep()) {
            display->drawFrame(cellX, y, cellWidth-1, 8);
        }
        
        if (isBeatActive) {
            if (isCurrentBeat && ch.isEnabled()) {
                display->drawBox(cellX+2, y+2, cellWidth-5, 4);
            } else {
                display->drawDisc(cellX + cellWidth/2, y+4, 2);
            }
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
