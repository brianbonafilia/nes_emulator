#include <iostream>
#include <cstdlib>
#include <cstdio>
#include "include/mapper.hpp"
#include "include/common.hpp"

Mapper::Mapper(u8* rom) : rom(rom){
    prgSize = rom[4] * 0x4000;
    chrSize = rom[5] * 0x2000;
    prgRamSize = rom[8] ? rom[8] * 0x2000 : 0x2000;


    prg = rom + 16;
    prgRam = new u8[prgRamSize];
    /* 
     *  note this is making the assumption that
     *  there is no trainer data,  which is only
     *  used for a few.
     */
    if (chrSize) {
        chr = rom + 16 + prgSize;
    }
    else {
        chrRam = true;
        chr = new u8[0x2000];

    }
}

Mapper::~Mapper() {
    delete rom;
    delete prgRam;
    if (chr) {
        delete chr;
    }
}

u8 Mapper::read(u16 addr){
    return rom[addr - 0x4000];
}

u8 Mapper::chr_read(u16 addr) {

}

template <int pageKBs>
void Mapper::map_prg(int slot, int bank) {

}

template <int pageKBs>
void map_chr(int slot, int bank) {

}


