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
    
    // Draw global BPM
    char buffer[32];
    sprintf(buffer, "%d BPM", state.bpm);
    bool isBpmSelected = (state.navLevel == GLOBAL && state.menuPosition == 0);
    drawParameter(0, 0, buffer, isBpmSelected, state.isEditing);
    
    // Draw channels
    for (uint8_t i = 0; i < 2; i++) {
        const MetronomeChannel& ch = state.getChannel(i);
        uint8_t y = 22 + (i * 20);
        
        // Channel header
        sprintf(buffer, "CH%d", i+1);
        bool isSelected = (state.navLevel == GLOBAL && state.menuPosition == i+1) ||
                         (state.navLevel == CHANNEL && state.activeChannel == i);
        drawParameter(0, y, buffer, isSelected, false);
        
        // Pattern info
        sprintf(buffer, "%d/%d P:%d", 
            ch.getCurrentBeat() + 1, 
            ch.getBarLength(), 
            ch.getPattern());
        drawText(64, y, buffer);
        
        // Draw beat grid
        drawChannelGrid(0, y+10, ch);
    }
    
    display->sendBuffer();
}

void Display::drawParameter(uint8_t x, uint8_t y, const char* text, bool selected, bool editing) {
    if (selected) {
        if (editing) {
            display->drawBox(x, y, 60, 12);
            display->setDrawColor(0);
            display->drawStr(x+2, y+10, text);
            display->setDrawColor(1);
        } else {
            display->drawFrame(x, y, 60, 12);
            display->drawStr(x+2, y+10, text);
        }
    } else {
        display->drawStr(x+2, y+10, text);
    }
}

void Display::drawChannelGrid(uint8_t x, uint8_t y, const MetronomeChannel& ch) {
    uint8_t cellWidth = 120 / ch.getBarLength();
    
    for (uint8_t i = 0; i < ch.getBarLength(); i++) {
        uint8_t cellX = x + (i * cellWidth);
        
        if (ch.getPatternBit(i)) {
            if (i == ch.getCurrentBeat() && ch.getBeatState()) {
                display->drawBox(cellX, y, cellWidth-1, 8);
            } else {
                display->drawFrame(cellX, y, cellWidth-1, 8);
            }
        } else {
            display->drawPixel(cellX + cellWidth/2, y + 4);
        }
    }
}
