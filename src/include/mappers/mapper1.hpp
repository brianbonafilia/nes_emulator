//
// Created by Brian Bonafilia on 7/9/21.
//
#pragma once
#include "../mapper.hpp"


class Mapper1 : public Mapper {
public:
    Mapper1(u8 *rom) : Mapper::Mapper(rom){
        mapperControl = 0 | (3 << 2);
        chrBank0 = 0;
        chrBank1 = 0;
        prgBank = 0;
        shifter = 0;
        shiftCount = 0;
        chrBanks = rom[5] ? rom[5] : 2;
        prgBanks = rom[4];

    }

    u8 read(u16 addr) override;

    u8 write(u16 addr, u8 v) override;

    u8 chr_read(u16 addr) override;

    u8 chr_write(u16 addr, u8 v) override;

private:
    // Registers

    u8 mapperControl;
    u8 chrBank0;
    u8 chrBank1;
    u8 prgBank;
    u8 shifter;
    u8 shiftCount;
    u8 chrBanks;
    u8 prgBanks;
};
