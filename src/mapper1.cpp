//
// Created by Brian Bonafilia on 7/9/21.
//

#include "include/mappers/mapper1.hpp"
#include "include/common.hpp"
#include "include/ppu.hpp"
#include <stdio.h>

u8 Mapper1::read(u16 addr) {
    u8 romReadMode = (mapperControl >> 2) & 0b11;

    if (romReadMode <= 1) {
        int index = addr - 0x8000;
        index = index + (prgBank * 0x8000);
        return prg[index];
    }
    if (addr >= 0xC000) {
        int index = addr - 0xC000;
        if (romReadMode == 3) {
            index = index + ((prgBanks-1) * 0x4000);
        } else {
            index = index + (prgBank * 0x4000);
        }
        return prg[index];
    } else if (addr >= 0x8000) {
        int index = addr - 0x8000;
        if (romReadMode == 2) {
            index = index + ((prgBanks-1) * 0x4000);
        } else {
            index = index + (prgBank * 0x4000);
        }
        return prg[index];
    }
    return Mapper::read(addr);
}

u8 Mapper1::write(u16 addr, u8 v) {
    if (addr >= 0x8000) {
        printf("writing to addr 0x%X  val:dd %X\n", addr, v);
        if (v > 0x80) {
            mapperControl |= 0x0C;
            shifter = 0;
            shiftCount = 0;
        } else {
            shifter = (shifter >> 1) | (v & 1 ? 0b10000 : 0);
            shiftCount++;
        }
        if (shiftCount == 5) {
            switch (addr) {
                case 0x8000 ... 0x9FFF:
                    mapperControl = shifter;
                    switch (mapperControl & 3) {
                        case 0:
                            printf("single screen NT 2\n");
                            printf("%X\n",mapperControl);
                            PPU::set_mirroring(PPU::singleLow);
                            break;
                        case 1:
                            printf("single screen NT 1\n");
                            PPU::set_mirroring(PPU::singleHigh);
                            break;
                        case 2:
                            PPU::set_mirroring(PPU::vertical);
                            break;
                        case 3:
                            PPU::set_mirroring(PPU::horizontal);
                            break;
                    }
                    break;
                case 0xA000 ... 0xBFFF:
                    chrBank0 = shifter;
                    break;
                case 0xC000 ... 0xDFFF:
                    chrBank1 = shifter;
                    break;
                case 0xE000 ... 0xFFFF:
                    prgBank = shifter;
                    break;
            }
            shifter = 0;
            shiftCount = 0;
        }
    } else {
        prgRam[addr - 0x6000] = v;
    }
    return v;
}

u8 Mapper1::chr_read(u16 addr) {
    u8 readMode = (mapperControl >> 4) & 1;
    if (readMode) {
        if (addr >= 0x1000) {
            return chr[0x1000 * chrBank0 + addr];
        } else {
            return chr[0x1000 * chrBank1 + addr];
        }
    } else {
       return chr[0x2000 * ((chrBank0 >> 1) & 0b1111) + addr];
    }
    return 0;
}

u8 Mapper1::chr_write(u16 addr, u8 v) {
    chr[addr] = v;
    return v;
}
