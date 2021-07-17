#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "include/cartridge.hpp"
#include "include/cpu.hpp"
#include "include/mapper.hpp"
#include "include/mappers/mapper0.hpp"
#include "include/mappers/mapper1.hpp"
#include "include/ppu.hpp"

namespace Cartridge {

    Mapper *mapper;
//Mapper will be include here to handle memory access

//access PRG ROM/RAM using mapper
    template<bool wr>
    u8 access(u16 addr, u8 v) {
        //TODO mapper access
        if (!wr) {
            return mapper->read(addr);
        } else {
            return mapper->write(addr, v);
        }
    }

    template<bool wr>
    u8 chr_access(u16 addr, u8 v) {
        //TODO mapper access
        if (!wr) {
            return mapper->chr_read(addr);
        } else {
            return mapper->chr_write(addr, v);
        }
        return 0;
    }

    void load(const char *fileName) {
        //Open to read binary file with ROM in it
        FILE *f = fopen(fileName, "rb");
        if (f == NULL) {
            fputs(fileName, stderr);
            fputs("File error", stderr);
            exit(1);
        }

        //jump to end of file, and get size, then go to start
        fseek(f, 0, SEEK_END);
        int size = ftell(f);
        fseek(f, 0, SEEK_SET);

        u8 *rom = new u8[size];

        int result = fread(rom, 1, size, f);
        if (result != size) {
            fputs("reading error", stderr);
            exit(2);
        }
        fclose(f);

        //Find Mapper
        u8 mapperID = (rom[7] & 0xF0) + (rom[6] >> 4);
        u8 nametableMirroring = rom[6] & 0x1;
        PPU::Mirroring mirroringType = nametableMirroring ? PPU::vertical : PPU::horizontal;

        if (mirroringType == PPU::vertical) {
            printf("vertical");
        } else {
            printf("horizontal");
        }


        std::cout << (int) mapperID << std::endl;

        switch (mapperID) {
            case 0:
                mapper = new Mapper0(rom);
                break;
            case 1:
                mapper = new Mapper1(rom);
                break;
        }
        //

        //Start running the ROM file
        CPU::power();
        PPU::power();
        PPU::set_mirroring(mirroringType);
        //TODO:  PPU start
    }

    bool loaded() {
        //TODO: check if it is loaded into proper mapper
        return true;
    }

    template u8 access<true>(u16, u8);
    template u8 access<false>(u16, u8);

    template u8 chr_access<true>(u16, u8);
    template u8 chr_access<false>(u16, u8);
} // namespace Cartridge
