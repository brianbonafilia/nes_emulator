#pragma once

#include "common.hpp"

namespace PPU {

    enum Mirroring {
        vertical, horizontal
    };

    void set_mirroring(Mirroring newMirroring);

    template<bool wr>
    u8 accessRegisters(u16 addr, u8 val = 0);

    template<bool wr>
    u8 access(u16 addr, u8 v = 0);

    void power();

    void doStep();

    int getCycle();

    int getScanline();

}
