//
// Created by Brian Bonafilia on 6/1/21.
//

#include "include/SDL2/SDL.h"
#include <iostream>
#include <stdio.h>
#include "unistd.h"
#include "include/ppu.hpp"
#include "include/gui.hpp"
#include "include/cartridge.hpp"
#include "include/cpu.hpp"

namespace PPU {
    bool debug = false;

    /**
     * PPU control 0x2000
     *
     *  flags by bit VPHB SINN
     *  V = NMI (non-maskable interrupt)
     *  P = PPU master/slave
     *  H = Sprite height
     *  B = background tile select
     *  S = sprite tile select
     *  I = increment mode
     *  NN = Nametable select
     */
    u8 ppuCtl;

    /**
     * PPU mask 0x2001
     *
     * flags by bit BGRs bMmG
     *
     * BRG = color emphasis
     * s = sprite enable
     * b = background enable
     * M = sprite left column enable
     * m = background left column enable
     * G = grey scale
     */
    u8 ppuMask;

    /**
     * PPU Status 0x2002
     *
     * flags by bit VSO- ----
     *
     * V = vblank
     * S = sprite 0 hit
     * O = sprite overflow
     */
    u8 ppuStatus;

    /**
     * Object Attribute Memory(OAM) address 0x2003
     *
     * the address to read/write to OAM
     */
    u8 oamAddr;

    /**
     * OAM data  0x2004
     *
     * Write OAM data here
     */
    u8 oamData;

    /**
     * fine scroll position 0x2005
     */
    u8 ppuScroll;

    /**
     * PPU Address 0x2006
     *
     * ppu read/write address
     */
    u8 ppuAddr;

    /**
     * PPU Data 0x2007
     *
     * ppu data read/write
     */
    u8 ppuData;

    /**
     * OAM Direct Memory Access 0x2008
     *
     * Used to transfer large amounts of data to OAM
     */
    u8 oamDma;

    u8 vRam[0x800];
    u8 palleteRam[0x20];

    /**
     * Object Attribute Memory
     */
    u8 OAM[0x100];

    /**
     * represents step we are on
     *
     *  261 or 260 unclear
     */
    int scanline;

    /**
     * Synonymous with dot, in some documentation.
     *
     * 341 of these per scanline
     */
    int cycle;

    /**
     * This is used to write 16 bit addresses through the 8-bit bus,
     * latch being active means we are on second half
     *
     * Reading from ppuStatus resets the latch
     */
    bool addressLatch;

    int getCycle() {
        return cycle;
    }

    int getScanline() {
        return scanline;
    }

    bool nmi_occured;

    Mirroring mirroring;

    u32 pixels[256*240];
    u32 sum = 0;
    u32 tenSeconds = 0;

    u32 pallete[] = {
            0x00545454, 0x00001e74, 0x00081090, 0x00300088,
            0x00440064, 0x005c0030, 0x00540400, 0x003c1800,
            0x00202a00, 0x00083a00, 0x00004000, 0x00003c00,
            0x0000323c, 0x00000000, 0x00989698, 0x00084cc4,
            0x003032ec, 0x005c1ee4, 0x008814b0, 0x00a01464,
            0x00982220, 0x00783c00, 0x00545a00, 0x00287200,
            0x00087c00, 0x00007628, 0x00006678, 0x00000000,
            0x00eceeec, 0x004c9aec, 0x00787cec, 0x00b062ec,
            0x00e454ec, 0x00ec58b4, 0x00ec6a64, 0x00d48820,
            0x00a0aa00, 0x0074c400, 0x004cd020, 0x0038cc6c,
            0x0038b4cc, 0x003c3c3c, 0x00eceeec, 0x00a8ccec,
            0x00bcbcec, 0x00d4b2ec, 0x00ecaeec, 0x00ecaed4,
            0x00ecb4b0, 0x00e4c490, 0x00ccd278, 0x00b4de78,
            0x00a8e290, 0x0098e2b4, 0x00a0d6e4, 0x00a0a2a0};

    /**
     * Internal registers of PPU
     *
     * vRamAddr = v (current VRAM addr, 15 bit)
     * temporaryVramAddr = t (temporary VRAM addr, 15  bit) or top left onscreen
     * fineXScroll = x (3 bits)
     * firstOrSecondWriteToggle = w (1 bit)
     *
     * Aside on 15,  when drawing background it updates the address to point
     * to nametable.  Bits 10-11 hold base address of nametable, 12-14 are Y offset
     * of scanline.
     */
    u16 vRamAddr, temporaryVramAddr;
    u8 fineXScroll;
    bool firstOrSecondWriteToggle;
    /**
     * These internal PPU registers contain the pattern table data
     *
     * The lower 8 bits are used,  while the upper 8 bits are loaded,  followed
     * by a shift
     */
    u16 bgLowShifter, bgHighShifter;
    /**
     * The contain pallete information for lower 8 bits of pattern table data
     */
    u8 bgAttributeLow, bgAttributeHigh;

    /**
     * latches which load into registers
     */
     u8 nametable, attributeByte, bgLow, bgHigh;

    /**
     * This is what I use to keep track of last address used
     */
    u16 renderingAddr;

    enum Mode {
        preRender, visible, postRender, verticalBlankLining
    };

    void set_mirroring(Mirroring newMirroring) {
        mirroring = newMirroring;
    }

    /**
     * If bits 3 or 4 are set, then we are rendering
     */
    bool rendering() {
        return ppuMask & 0x18;
    }

    /**
     * Get the index into our vRAM (video RAM) using nametable mirroring
     */
    u16 get_nametable_mirroring(u16 addr) {
        switch (mirroring) {
            case horizontal:
                if (addr > 0x1FFF && addr < 0x2400) {
                    addr = addr - 0x400;
                } else if (addr > 0x2BFF) {
                    addr = addr - 0x400;
                }

                if (addr < 0x2400) {
                    return addr - 0x2000;
                } else {
                    return addr - 0x2400;
                }
            case vertical:
                return addr % 800;
            default:
                return addr % 800;
        }
    }

    u8 ppu_read(u16 addr) {
        switch (addr) {
            case 0x0000 ... 0x1FFF:
                // return from pattern table 0 and 1
                return Cartridge::chr_access<false>(addr);
            case 0x2000 ... 0x2FFF:
                return vRam[get_nametable_mirroring(addr)];
            case 0x3000 ... 0x3EFF:
                // mirror of 0x2000 ... 2EFF
                //std::cout << "WOW! really didn't expect to be here" << std::endl;
                return 0;
            case 0x3F00 ... 0x3FFF:
                return palleteRam[(addr - 0x3F00) % 0x20];
        }
        return 0;
    }

    u8
    ppu_write(u16 addr, u8 value) {
        switch(addr) {
            case 0x2000 ... 0x2FFF:
                //printf("oh lowrd 0x%x, value 0x%x \n", addr, value);
                return vRam[get_nametable_mirroring(addr)] = value;
            case 0x3F00 ... 0x3FFF:
                //printf("Writing to palleteAddr 0x%x, value %d \n",
                       //(addr - 0x3F00) % 0x20, value);
                return palleteRam[(addr - 0x3F00) % 0x20] = value;
            default:
                printf("fucl \n");
                //exit(1);
        }
        return 0;
    }

    void loadShifters() {
        bgAttributeLow = attributeByte & 1 ? 0xFF : 0x00;
        bgAttributeHigh = attributeByte & 2 ? 0xFF : 0x00;
        bgLowShifter = ((bgLowShifter << 8) & 0xFF00) | bgLow;
        bgHighShifter = ((bgHighShifter << 8) & 0xFF00) | bgHigh;
    }

    /**
     * Use vramAddr register to get current name table byte addr
     */
    u16 getNametableByteAddr() {
        return 0x2000 | (vRamAddr & 0x0FFF);
    }

    /**
     * Get attribute byte addr,  this is found by taking the tile we are viewing
     * and using nametable attribute, getting the byte associated with the
     * current 2x2 tile or 16x16 pixel.
     *
     * TODO: this formula was found on nesdev,  but I don't fully grok it,
     *  why does this bit shifting trick give us the right attribute byte
     */
    u16 getAttributeByteAddr() {
        return 0x23C0 | (vRamAddr & 0x0C00)
                        | ((vRamAddr >> 4) & 0x38)
                        | ((vRamAddr >> 2) & 0x07);
    }

    /**
     * 32x30 tiles,  each vramAddr represents a tile.
     */
    u16 getPatternTableLowAddr() {
        u16 xPos = (u16) nametable << 4;
        u16 HalfBit = (ppuCtl & 0x08)  << 8;
        return HalfBit | xPos | (vRamAddr & 0x7);
    }

    void writePixel() {
        u16 mask = 0x8000 >> fineXScroll;
        u16 att = 0;//(bgAttributeHigh & (0x80 >> fineXScroll) ? 2 : 1) + (bgAttributeLow & (0x80 >> fineXScroll) ? 1 : 0);
        u16 val = (bgLowShifter & mask ? 1 : 0) + (bgHighShifter & mask ? 2 : 0);
        att >>= fineXScroll;
        att *= 4;
        val += att;
        fineXScroll++;
        fineXScroll %= 8;

        u8 color = ppu_read(0x3F00 + val);
        if (!rendering()) {
            color = 0;
        }
        if (debug) {
            std::cout << "mask is " << std::bitset<16>(mask) << std::endl;
            std::cout << "bgPatternHigh is " << std::bitset<16>(bgHighShifter) << std::endl;
            std::cout << "bgPatternLow is " << std::bitset<16>(bgLowShifter) << std::endl;
//            std::cout << "bgAttributeLow is " << std::hex << (int) bgAttributeLow << std::endl;
//            std::cout << "bgAttributeHigh is " << std::hex << (int) bgAttributeHigh << std::endl;
            std::cout << "fineX is " << std::to_string(fineXScroll) << std::endl;
            printf("value is %x  color is %x \n", val, color);
            printf("vRamAddr is %x \n", vRamAddr & 0x3FFF);
        }
        pixels[scanline * 240 + cycle] = color;
    }

    void shiftHorizontal() {
        if (!rendering()) {
            return;
        }
        if ((vRamAddr & 0X1F) == 31) {
            vRamAddr &= ~0x1F;
            vRamAddr ^= 0x400;
        } else {
            //printf("coarseX is %d , cycle is %d, scanline is %d\n", vRamAddr & 0X1F, cycle, scanline);
            vRamAddr++;
        }
        //int coarseY = (vRamAddr & 0x3E0) >> 5;
        //printf("horizontal coarseY is : %d \n", coarseY);
    }

    void shiftVertical() {
        //printf("fart \n");
        int fineY = (vRamAddr & 0x7000) >> 12;
        //printf("fineY is : %d \n", fineY);
        //printf("0x%x \n", vRamAddr);
        if (!rendering()) {
            return;
        }
        if ((vRamAddr & 0x7000) != 0x7000) {  //fine y < 7
            vRamAddr += 0x1000;               // increment fine u
        } else {
            vRamAddr &= ~0x7000;             // otherwise increment coarse y
            int coarseY = (vRamAddr & 0x3E0) >> 5;
            //printf("coarseY is : %d \n", coarseY);
            if (coarseY == 29) {
                coarseY = 0;
                vRamAddr ^= 0x0800;
            } else if (coarseY == 31) {
                coarseY = 0;
            } else {
                coarseY++;
            }
            vRamAddr = (vRamAddr & ~0x3E0) | (coarseY << 5);
            coarseY = (vRamAddr & 0x3E0) >> 5;
            //printf("new coarseY is : %d \n", coarseY);
        }
        fineY = (vRamAddr & 0x7000) >> 12;
        //printf("fineY is : %d \n", fineY);
        //printf("new vram addr 0x%x \n", vRamAddr);
    }

    void scan_line() {
        if (scanline > -1 && scanline < 241) {
            writePixel();
        }
        if (scanline == 241 && cycle == 1) {
            if (ppuCtl & 0x80) {
                CPU::set_nmi();
            }
        }
        switch (cycle % 8) {
            case 0:
                loadShifters();
                break;
            case 1:
                renderingAddr = getNametableByteAddr();
                //printf("Nametable byte addr %x \n", renderingAddr);
                nametable = ppu_read(renderingAddr);
                break;

            case 3:
                //printf("Attribute byte addr %x \n", renderingAddr);
                renderingAddr = getAttributeByteAddr();
                attributeByte = ppu_read(renderingAddr);
                break;
            case 5:
                renderingAddr = getPatternTableLowAddr();
                //printf("Patterntable byte addr %x \n", renderingAddr);
                bgLow = ppu_read(renderingAddr);
                break;
            case 7:
                renderingAddr += 8;
                bgHigh = ppu_read(renderingAddr);
                shiftHorizontal();
                break;
        }
        if (cycle == 256) {
            shiftVertical();
        }
        if (cycle == 257) {
            vRamAddr = vRamAddr | (temporaryVramAddr & 0x41F);
        } else if (cycle > 280 && cycle < 304) {
            vRamAddr = vRamAddr | (temporaryVramAddr & 0x7be0);
        }
        /*td::cout << "this is current vramAddr " <<  std::hex << vRamAddr << std::endl;
        std::cout << "this is current renderingAddr " <<  std::hex << renderingAddr << std::endl;*/
        return;
    }

    void drawNametable() {
        for (int i = 0; i < 2024; i++) {
            printf("%02x ", ppu_read(0x2000 + i));
            if (i % 16 == 15) {
                printf("\n");
            }
        }
    }

    void printPatternTable(int addr) {
        for (int i = 0; i < 8; i++) {
            std::cout << std::bitset<8>(ppu_read(addr + i)) << "\t";
            std::cout << std::bitset<8>(ppu_read(addr + 8 + i)) << std::endl;
        }
    }

    void drawPatterns() {
        palleteRam[1] = 10;
        palleteRam[0] = 20;
        palleteRam[3] = 30;
        palleteRam[2] = 40;
        //drawNametable();
        for (int tileRow = 0; tileRow < 30; tileRow++) {
            for (int tile = 0; tile < 32; tile++) {
//                int tR = 6;
//                int t = 6;
//                u16 lowByteAddr = ppu_read(0x2000 + 32 * tR + t);
//                u16 highByteAddr = lowByteAddr + 8;
                u16 lowByteAddr = (tileRow * 32 * 8) + (tile * 16);
                u16 highByteAddr = lowByteAddr + 8;

                for (int row = 0; row < 8; row++) {
                    //printf("\n");
                    for (int col = 0; col < 8; col++) {
//                        lowByteAddr += row;
//                        highByteAddr += row;
                        u8 lowByte = Cartridge::chr_access<false>(lowByteAddr + row);
                        u8 highByte = Cartridge::chr_access<false>(highByteAddr + row);
                        u8 mask = 0x80 >> col % 8;
                        u8 lowBit = lowByte & mask ? 1 : 0;
                        u8 highBit = highByte & mask ? 2 : 0;
                        u8 color = lowBit + highBit;
                        pixels[256 * (row + tileRow * 8) + (col + tile * 8)] = pallete[palleteRam[color]];
                    }
                }
            }
        }
    }

    void doStep() {
        cycle++;
        scan_line();
        if (cycle == 341) {
            cycle = 0;
            scanline++;
        }
        if (scanline == 241 && cycle == 1) {
            // set vblank on ppuStatus
            ppuStatus |= 0x80;
            //vRamAddr = temporaryVramAddr;
            //std::cout << "well it happened :/ " << std::to_string(ppuStatus) << std::endl;
        }
        if (scanline == 260) {
            scanline = 0;
            drawPatterns();
            GUI::update_frame(pixels);
            if (tenSeconds++ > 60) {
                tenSeconds = 0;
                sum = ++sum % 64;
            }
        }
    }

    template<bool wr>
    u8 accessRegisters(u16 addr, u8 val) {
        //std::cout << "access register " << std::hex << addr << std::endl;
        u8 num;
        u16 index = (addr - 0x2000) % 8;
        if (index != 9) {
            std::cout << "access register " << std::to_string(index) << std::endl;
            printf("register addr %x \n", addr);
            if (wr) {
                printf("writing new value 0x%x \n", val);
            }
        }
        if (wr) {
            switch (index) {
                case 0:
                    ppuCtl = val;
                    if (val == 0x7F) {
                        exit(0);
                    }
                    return ppuCtl;
                case 1:
                    ppuMask = val;
                    return ppuMask;
                case 3:
                    oamAddr = val;
                    return oamAddr;
                case 4:
                    OAM[oamAddr] = val;
                    return val;
                case 5:
                    if (!addressLatch) {
                        fineXScroll = val & 0x7;
                        temporaryVramAddr |= (val & 0xF8);
                    } else {
                        u16 fineY = val & 0x7;
                        fineY <<= 9;
                        u16 coarseY = val & 0xF8;
                        coarseY <<= 5;
                        temporaryVramAddr |= (fineY | coarseY);
                    }
                    addressLatch = !addressLatch;
                    return val;
                case 6:
                    // TODO: use temporary vram addr and do appropriate swap
                    if (!addressLatch) {
                        temporaryVramAddr = (temporaryVramAddr & 0xFF) | (u16) (0x3F & val) << 8;
                        std::cout << "temporary vram " << std::bitset<16>(temporaryVramAddr) << std::endl;
                        std::cout << "new val " << std::bitset<8>(val) << std::endl;
                    } else {
                        temporaryVramAddr = (temporaryVramAddr & 0xFF00) | val;
                        std::cout << "temporary vram " << std::bitset<16>(temporaryVramAddr) << std::endl;
                        std::cout << "new val " << std::bitset<8>(val) << std::endl;
                        vRamAddr = temporaryVramAddr;
                        printf("new vramAddr is 0x%x \n", vRamAddr);
                    }
                    addressLatch = !addressLatch;
                    return val;
                case 7:
                    if (ppuStatus & 0x8) {
                        return 0;
                    }
                    printf("writing to ppuAddr 0x%x \n", vRamAddr&0x3FFF);
                    num =  ppu_write(vRamAddr & 0x3FFF, val);
                    vRamAddr += ppuCtl & 0x4 ? 32 : 1;
                    return num;
                default:
                    return 0;
            }
        }
        switch (index) {
            case 2:
                num = ppuStatus;
                ppuStatus &= 0x7F;

                addressLatch = false;
                return num;
            case 4:
                return OAM[oamAddr++];
            case 7:
                num = ppu_read(vRamAddr&0x3FFF);
                vRamAddr += ppuCtl & 0x4 ? 32 : 1;
                return num;
            default:
                return 0;
        }
    }

    template u8 accessRegisters<true>(u16 addr, u8 val);
    template u8 accessRegisters<false>(u16 addr, u8 val);

    void power() {
        ppuCtl = 0;
        ppuMask = 0;
        ppuStatus = 0;
        oamAddr = 0;
        scanline = 0;
        fineXScroll = 0;
        addressLatch = false;

        scanline = 0;
        cycle = 0;

        memset(vRam, 0xFF, sizeof(vRam));
        memset(OAM, 0xFF, sizeof(OAM));
        memset(palleteRam, 0xFF, sizeof(palleteRam));
    }
}


