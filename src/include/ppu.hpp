#pragma once

#include "common.hpp"

namespace PPU {

    enum Mirroring {
        vertical, horizontal, singleLow, singleHigh
    };

    void set_mirroring(Mirroring newMirroring);

    template<bool wr>
    u8 accessRegisters(u16 addr, u8 val = 0);

    template<bool wr>
    u8 access(u16 addr, u8 v = 0);

    void power();

    void doStep();

    /**
     * transfer 256 byte data directly to OAM
     */
    void transferToOamDma(u8 dataTransfer, int index);

    int getCycle();

    int getScanline();

}
