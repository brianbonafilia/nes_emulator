#include <iostream>
#include <cstdio>
#include "include/mapper.hpp"
#include "include/common.hpp"

Mapper::Mapper(u8 *rom) : rom(rom) {
    prgSize = rom[4] * 0x4000;
    chrSize = rom[5] * 0x2000;
    prgRamSize = rom[8] ? rom[8] * 0x2000 : 0x2000;

    std::cout << (int) prgSize << " is the size of prg" << std::endl;
    printf("size of chr size is %d\n", chrSize);
    printf("size of prg ram size is %d", prgRamSize);

    prg = rom + 16;
    prgRam = new u8[prgRamSize];
    /*
     *  note this is making the assumption that
     *  there is no trainer data,  which is only
     *  used for a few.
     */
    if (chrSize) {
        chr = rom + 16 + prgSize;
    } else {
        printf("ITs RAMMMMM");
        chrRam = true;
        chrSize = 0x2000;
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

u8 Mapper::read(u16 addr) {
//    printf("ADDR is %X \n", addr);

    if (addr >= 0x8000) {
//        printf("%X ", (addr - 0x8000) % prgSize);
//        printf("val %X \n", prg[(addr - 0x8000) % prgSize]);
        return prg[(addr - 0x8000) % prgSize];
    } else {
        printf("GMA halp");
        return 0;
    }
}

u8 Mapper::chr_read(u16 addr) {
    return chr[addr % chrSize];
}

template<int pageKBs>
void Mapper::map_prg(int slot, int bank) {
}

template<int pageKBs>
void Mapper::map_chr(int slot, int bank) {
}
