#pragma once
#include <U8g2lib.h>
#include "MetronomeState.h"
#include "MetronomeChannel.h"

class Display {
private:
    U8G2_SH1106_128X64_NONAME_F_HW_I2C* display;
    
    void drawGlobalRow(const MetronomeState& state);
    void drawGlobalProgress(const MetronomeState& state);
    void drawChannelBlock(const MetronomeState& state, uint8_t channelIndex, uint8_t y);
    void drawBeatGrid(uint8_t x, uint8_t y, const MetronomeChannel& ch, bool isEditing);
    void drawFlash(uint32_t currentTime);

public:
    Display();
    void begin();
    void update(const MetronomeState& state);
};