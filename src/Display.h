#pragma once
#include <U8g2lib.h>
#include "MetronomeState.h"
#include "MetronomeChannel.h"

class Display {
    private:
        U8G2_SH1106_128X64_NONAME_F_HW_I2C* display;

    public:
        Display();
        void begin();
        void update(const MetronomeState& state);

    private:
        void drawBPM(const MetronomeState& state);
        void drawBar(const MetronomeState& state);
        void drawPattern(const MetronomeState& state);
        void drawGrid(const MetronomeState& state);
        void drawParameter(uint8_t x, uint8_t y, const char* text, bool selected, bool editing);
        void drawChannelGrid(uint8_t x, uint8_t y, const MetronomeChannel& ch);
        void drawText(uint8_t x, uint8_t y, const char* text) {
            display->drawStr(x, y, text);
        }
};
