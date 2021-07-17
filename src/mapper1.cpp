//
// Created by Brian Bonafilia on 7/9/21.
//

#include "include/mappers/mapper1.hpp"
#include "include/common.hpp"
#include <stdio.h>


u8 Mapper1::read(u16 addr) {
    u8 romReadMode = (mapperControl >> 2) & 0b11;
//    printf("ROM read mode is %d, prgBank is %d\n", romReadMode, prgBank);

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
    printf("writing to addr 0x%X  val: %X\n", addr, v);
    if (addr >= 0x8000) {
        if (v > 0x80) {
            shifter = 0;
            shiftCount = 0;
        } else {
            shifter = (shifter << 1) | (v & 1);
            shiftCount++;
        }
        if (shiftCount == 5) {
            switch (addr) {
                case 0x8000 ... 0x9FFF:
                    mapperControl = shifter;
                    printf("It's happening\n");
                    break;
                case 0xA000 ... 0xBFFF:
                    chrBank0 = shifter;
                    printf("It's happening\n");
                    break;
                case 0xC000 ... 0xDFFF:
                    chrBank1 = shifter;
                    printf("It's happening\n");
                    break;
                case 0xE000 ... 0xFFFF:
                    printf("It's happening\n");
                    prgBank = shifter;
                    break;
            }
            shifter = 0;
            shiftCount = 0;
        }
    } else {
        printf("say hit");
    }
    return 0;
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
//        printf("chrBank is %d\n", chrBank0);
       return chr[0x2000 * ((chrBank0 >> 1) & 0b1111) + addr];
    }
    return 0;
}

u8 Mapper1::chr_write(u16 addr, u8 v) {
    printf("it's happening");
    return Mapper::chr_write(addr, v);
}
