//
// Created by Brian Bonafilia on 6/1/21.
//

#include "include/SDL2/SDL.h"
#include <iostream>
#include <stdio.h>
#include "include/ppu.hpp"
#include "include/gui.hpp"
#include "include/cartridge.hpp"
#include "include/cpu.hpp"

namespace PPU {

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
     * Secondary OAM buffer
     */
     u8 secondaryOamBuffer[0x40];

     u8 spriteIndex;
     u8 coordinateIndex;
     u8 secondaryOamIndex;

     u8 spritePatterns[16];
     u8 counters[8];
     u8 attributeLatches[8];
     u8 spriteIndices[8];

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

    u32 pallete[] = {
            0x00545454, 0x00001e74, 0x00081090, 0x00300088,0x00440064, 0x005c0030, 0x00540400, 0x003c1800,
            0x00202a00, 0x00083a00, 0x00004000, 0x00003c00,0x0000323c, 0x00000000, 0x00000000, 0x00000000,
            0x00989698, 0x00084cc4,0x003032ec, 0x005c1ee4, 0x008814b0, 0x00a01464, 0x00982220, 0x00783c00,
            0x00545a00, 0x00287200,0x00087c00, 0x00007628, 0x00006678, 0x00000000, 0x00000000, 0x00000000,
            0x00eceeec, 0x004c9aec, 0x00787cec, 0x00b062ec,0x00e454ec, 0x00ec58b4, 0x00ec6a64, 0x00d48820,
            0x00a0aa00, 0x0074c400, 0x004cd020, 0x0038cc6c,0x0038b4cc, 0x003c3c3c, 0x00000000, 0x00000000,
            0x00eceeec, 0x00a8ccec,0x00bcbcec, 0x00d4b2ec, 0x00ecaeec, 0x00ecaed4,0x00ecb4b0, 0x00e4c490,
            0x00ccd278, 0x00b4de78,0x00a8e290, 0x0098e2b4, 0x00a0d6e4, 0x00a0a2a0, 0x00000000, 0x00000000};

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
    u16 bgAttributeLow, bgAttributeHigh;

    /**
     * latches which load into registers
     */
     u8 nametable, attributeByte, bgLow, bgHigh;

    /**
     * This is what I use to keep track of last address used
     */
    u16 renderingAddr;

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
                if (addr > 0x2400 && addr < 0x2800) {
                    addr = addr - 0x400;
                } else if (addr > 0x2BFF) {
                    addr = addr - 0x400;
                }
                if (addr < 0x2400) {
                    addr = addr - 0x2000;
                } else {
                    addr = addr - 0x2400;
                }
                break;
            case vertical:
                addr =  addr % 0x800;
                break;
        }
        return addr;
    }

    u8 ppu_read(u16 addr) {
        u8 palleteIndex;
        switch (addr) {
            case 0x0000 ... 0x1FFF:
                // return from pattern table 0 and 1
                return Cartridge::chr_access<false>(addr);
            case 0x2000 ... 0x2FFF:
                return vRam[get_nametable_mirroring(addr)];
            case 0x3000 ... 0x3EFF:
                return ppu_read(addr - 0x1000);
            case 0x3F00 ... 0x3FFF:
                palleteIndex = (addr - 0x3F00) % 0x20;
                if (palleteIndex % 4 == 0 && palleteIndex >= 0x10) {
                    palleteIndex -= 0x10;
                }
                return palleteRam[palleteIndex];
        }
        return 0;
    }

    void drawNametable() {
        int sl = 0;
        printf("scanline %04d   ", sl * 8);
        for (int i = 0; i < 1024; i++) {
            printf("%02X ", ppu_read(0x2000 + i));
            if (i % 32 == 31) {
                sl++;
                printf("\nscanline %04d   ", sl * 8);
            }
        }
    }

    u8
    ppu_write(u16 addr, u8 value) {
        u8 palleteIndex;
        switch(addr) {
            case 0x0000 ... 0x1FFF:
                // return from pattern table 0 and 1
                return Cartridge::chr_access<true>(addr);
            case 0x2000 ... 0x2FFF:
                return vRam[get_nametable_mirroring(addr)] = value;
            case 0x3000 ... 0x3EFF:
                return ppu_write(addr - 0x1000, value);
            case 0x3F00 ... 0x3FFF:
                palleteIndex = (addr - 0x3F00) % 0x20;
                if (palleteIndex % 4 == 0 && palleteIndex >= 0x10) {
                    palleteIndex -= 0x10;
                }
                return palleteRam[palleteIndex] = value;
            default:
                printf("fucl addr %X\n", addr);
        }
        return 0;
    }

    void transferToOamDma(u8 dataTransfer, int index) {
//        if (OAM[index] != dataTransfer) {
//            printf("its mofucking happening");
//        }
        OAM[index] = dataTransfer;
    }

    void loadShifters() {
        u16 coarseY = (vRamAddr & 0x3E0) >> 5;
        u16 coarseX = vRamAddr & 0x1F;
        u8 shiftAmount = 0;
        if ((coarseX-1) % 4 > 1) {
            shiftAmount += 2;
        }
        if ((coarseY) % 4 > 1) {
            shiftAmount += 4;
        }
        bgAttributeLow = (bgAttributeLow & 0xFF00) | (attributeByte & (1 << shiftAmount) ? 0xFF : 0x00);
        bgAttributeHigh = (bgAttributeHigh & 0xFF00) | (attributeByte & (2 << shiftAmount) ? 0xFF : 0x00);
        bgLowShifter = (bgLowShifter & 0xFF00) | bgLow;
        bgHighShifter = (bgHighShifter & 0xFF00) | bgHigh;
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

    u16 getSpriteTableLowAddr(u8 tileIndex, u8 yPos) {
        u16 tilePos = (u16) tileIndex << 4;
        u16 spriteTableSelect = (ppuCtl & 0x8) ? 0x1000 : 0;
        u16 fineY = (scanline - yPos);
        return spriteTableSelect | tilePos | fineY;
    }

    /**
     * 32x30 tiles,  each vramAddr represents a tile.
     */
    u16 getPatternTableLowAddr() {
        u16 tilePos = (u16) nametable << 4;
        u16 patternTableSelect = (ppuCtl & 0x10)  << 8;
        u16 fineY = (0x7000 & vRamAddr) >> 12;
        //printf("tilePos: %d,  patterntableSelect %X,  fineY %d", tilePos, patternTableSelect, fineY);
        return patternTableSelect | tilePos | fineY;
    }


    void shiftShifters() {
        bgAttributeLow <<= 1;
        bgAttributeHigh <<= 1;
        bgLowShifter <<= 1;
        bgHighShifter <<= 1;
    }

    void writePixel() {
        u16 mask = 0x8000 >> fineXScroll;
        u16 att = (bgAttributeHigh & mask ? 2 : 0) + (bgAttributeLow & mask  ? 1 : 0);
        u16 val = (bgLowShifter & mask ? 1 : 0) + (bgHighShifter & mask ? 2 : 0);
        if (val != 0) {
            att *= 4;
            val += att;
        }

        u8 color = ppu_read(0x3F00 + val);
        if (!rendering()) {
            color = 0;
        }
        u8 spriteColor = 0;
        for (int sprite = 0; sprite < 8; sprite++) {
            if(counters[sprite] == 0 && (spritePatterns[sprite * 2] || spritePatterns[sprite * 2 + 1])) {
                u8 spriteMask = attributeLatches[sprite] & 0x40 ? 0x1 : 0x80;
                spriteColor = (spriteMask & spritePatterns[sprite * 2] ? 1 : 0)
                        + (spriteMask & spritePatterns[sprite * 2 + 1] ? 2 : 0);
                u8 spriteAttr = attributeLatches[sprite] & 0x3;
                if (spriteColor != 0) {
                    spriteColor = 4 * (4 + spriteAttr) + spriteColor;
                    if (spriteIndices[sprite] == 0 && val != 0) {
                        ppuStatus |= 0x40;
                    }

                }
                if (spriteMask == 0x1) {
                    spritePatterns[sprite * 2] >>= 1;
                    spritePatterns[sprite * 2 + 1] >>= 1;
                } else {
                    spritePatterns[sprite * 2] <<= 1;
                    spritePatterns[sprite * 2 + 1] <<= 1;
                }
            } else {
                counters[sprite]--;
            }
        }
        if (spriteColor != 0) {
            color = ppu_read(0x3F00 + spriteColor);
        }
        pixels[scanline * 256 + cycle] = pallete[color];
        shiftShifters();
    }

    void evaluateSprites() {
        switch (cycle) {
            case 1 ... 64:
                secondaryOamBuffer[cycle - 1] = 0xFF;
                break;
            case 65 ... 256:
                if (cycle == 65) {
                    spriteIndex = 0;
                    coordinateIndex = 0;
                    secondaryOamIndex = 0;
                }
                if (cycle % 2 == 0) {
                    u8 y;
                    switch (coordinateIndex) {
                        case 0:
                            y = OAM[(spriteIndex * 4) % 0x100];
                            if (scanline >= y && scanline < y + 8) {
                                if (secondaryOamIndex < 32) {
                                    spriteIndices[secondaryOamIndex / 4] = spriteIndex;
                                }
                                secondaryOamBuffer[secondaryOamIndex++] = y;
                                coordinateIndex++;
                            } else {
                                spriteIndex++;
                            }
                            break;
                        case 1 ... 3:
                            u8 val = OAM[(spriteIndex * 4) + coordinateIndex];
                            secondaryOamBuffer[secondaryOamIndex++] = val;
                            coordinateIndex = coordinateIndex + 1;
                            if (coordinateIndex == 4) {
                                spriteIndex++;
                                coordinateIndex = 0;
                            }
                            break;
                    }
                    spriteIndex %= 64;
                    secondaryOamIndex %= 64;
                }
                break;
            case 257 ... 320:
                u8 sprite = (cycle - 257) / 8;
                if (cycle % 8 == 0) {
                    if (sprite * 4 >= secondaryOamIndex) {
                        counters[sprite] = 0xFF;
                        spritePatterns[sprite * 2] = 0x00;
                        spritePatterns[sprite * 2 +1] = 0x00;
                    } else {
                        counters[sprite] = secondaryOamBuffer[sprite * 4 + 3];
                        attributeLatches[sprite] = secondaryOamBuffer[sprite * 4 + 2];
                        u16 lowAddr = getSpriteTableLowAddr(
                                secondaryOamBuffer[sprite * 4 + 1],secondaryOamBuffer[sprite * 4]);
                        spritePatterns[sprite * 2] = ppu_read(lowAddr);
                        spritePatterns[sprite * 2 + 1] = ppu_read(lowAddr + 8);
                    }
                }

        }
    }

    void shiftHorizontal() {
        if (!rendering()) {
            return;
        }
        if ((vRamAddr & 0X1F) == 31) {
            vRamAddr &= ~0x1F;
            vRamAddr ^= 0x400;
        } else {
            vRamAddr++;
        }
    }

    void shiftVertical() {
        int fineY = (vRamAddr & 0x7000) >> 12;
//        printf("fineY is : %d  cycle is %d scanline is %d\n", fineY, cycle, scanline);
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
//            printf("new coarseY is : %d \n", coarseY);
        }
        fineY = (vRamAddr & 0x7000) >> 12;
//        printf("fineY is : %d \n", fineY);
//        printf("new vram addr 0x%x \n", vRamAddr);
    }

    bool isVisibleScanline() {
        return scanline > -1 && scanline < 240;
    }

    bool isVisibleCycle() {
        return cycle > 0 && cycle < 257;
    }

    bool isVerticleBlankingScanline() {
        return scanline > 240 && scanline < 261;
    }

    void setInterruptToCpuIfNeeded() {
        if (scanline == 241 && cycle == 1) {
            if (ppuCtl & 0x80) {
                CPU::set_nmi();
            }
        }
    }

    /**
     * perform one step for the current scanline
     */
    void scan_line() {
        setInterruptToCpuIfNeeded();
        if (isVisibleScanline()) {
            evaluateSprites();
        }
        if (isVisibleCycle() && isVisibleScanline()) {
            writePixel();
        }
        if (isVerticleBlankingScanline()) {
            return;
        }
        switch (cycle % 8) {
            case 0:
                if (cycle < 256) {
                    loadShifters();
                }
                break;
            case 1:
                renderingAddr = getNametableByteAddr();
                nametable = ppu_read(renderingAddr);
                break;

            case 3:
                renderingAddr = getAttributeByteAddr();
                attributeByte = ppu_read(renderingAddr);
                break;
            case 5:
                renderingAddr = getPatternTableLowAddr();
                bgLow = ppu_read(renderingAddr);
                break;
            case 7:
                renderingAddr += 8;
                bgHigh = ppu_read(renderingAddr);
                if (cycle < 256 || cycle > 320) {
                    shiftHorizontal();
                }
                break;
        }
        if (cycle == 256) {
            shiftVertical();
        }
        if (cycle == 257 && rendering()) {
            vRamAddr = (vRamAddr & ~0x041F) | (temporaryVramAddr & 0x41F);
        } else if (rendering() && cycle > 280 && cycle < 304 && scanline == 261) {
            vRamAddr = (vRamAddr & ~0x7BE0) | (temporaryVramAddr & 0x7be0);
        }
    }

    void printPatternTable(int addr) {
        for (int i = 0; i < 8; i++) {
            std::cout << std::bitset<8>(ppu_read(addr + i)) << "\t";
            std::cout << std::bitset<8>(ppu_read(addr + 8 + i)) << std::endl;
        }
    }

    void drawAttrTable() {
        for (int i = 0; i < 64; i++) {
            printf("%02X ", ppu_read(0x23C0 + i));
            if (i % 8 == 7) {
                printf("\n");
            }
        }
    }

    void drawPatterns() {
        drawNametable();
        drawAttrTable();
        for (int tileRow = 0; tileRow < 30; tileRow++) {
            for (int tile = 0; tile < 32; tile++) {
                u16 attrAddr = 0x23C0 + ((tileRow / 4) * 8) + (tile / 4);
                u8 attrByte = ppu_read(attrAddr);
                u8 mask = 3;
                u8 shiftAmount = 0;
                if (tileRow % 8 > 3) {
                    shiftAmount += 4;
                } if (tile % 8 > 3) {
                    shiftAmount += 2;
                }
                u8 attrVal = ((mask << shiftAmount) & attrByte) >> shiftAmount;
                u16 lowByteAddr = ppu_read(0x2000 + 32 * tileRow + tile);
                lowByteAddr = 0x1000 + (lowByteAddr) * 16;
                u16 highByteAddr = lowByteAddr + 8;
                for (int row = 0; row < 8; row++) {
                    for (int col = 0; col < 8; col++) {
                        u8 lowByte = Cartridge::chr_access<false>(lowByteAddr + row);
                        u8 highByte = Cartridge::chr_access<false>(highByteAddr + row);
                        u8 mask = 0x80 >> col % 8;
                        u8 lowBit = lowByte & mask ? 1 : 0;
                        u8 highBit = highByte & mask ? 2 : 0;
                        u8 color = lowBit + highBit;
                        pixels[256 * (row + tileRow * 8) + (col + tile * 8)] = pallete[attrVal * 0 + palleteRam[color]];
                    }
                }
            }
        }
    }

    void doStep() {
        scan_line();
        cycle++;
        if (cycle == 341) {
            cycle = 0;
            scanline++;
        }
        if (scanline == 241 && cycle == 1) {
            // set vblank on ppuStatus
            ppuStatus |= 0x80;
        }
        if (scanline == 261 && cycle == 2) {
            ppuStatus &= ~0x40;
        }
        if (scanline > 261) {
            scanline = 0;
//            drawPatterns();
            GUI::update_frame(pixels);
        }
    }

    template<bool wr>
    u8 accessRegisters(u16 addr, u8 val) {
        u8 num;
        u16 index = (addr - 0x2000) % 8;
        u16 nametableVal;
        u16 mask;
        if (wr) {
            switch (index) {
                case 0:
                    ppuCtl = val;
                    nametableVal = (val & 0x3) << 10;
                    mask = 3 << 10;
                    temporaryVramAddr &= ~mask;
                    temporaryVramAddr |= nametableVal;
                    return val;
                case 1:
                    ppuMask = val;
                    return val;
                case 3:
                    oamAddr = val;
                    return val;
                case 4:
                    OAM[oamAddr] = val;
                    return val;
                case 5:
                    if (!addressLatch) {
                        fineXScroll = val & 0x7;
                        temporaryVramAddr &= ~0x1f;
                        temporaryVramAddr |= (val & 0xF8) >> 3;
                    } else {
                        u16 fineY = val & 0x7;
                        fineY <<= 12;
                        u16 coarseY = (val & 0xF8) >> 3;
                        coarseY <<= 5;
                        temporaryVramAddr &= ~0x73e0;
                        temporaryVramAddr |= (fineY | coarseY);
                    }
                    addressLatch = !addressLatch;
                    return val;
                case 6:
                    if (!addressLatch) {
                        temporaryVramAddr = (temporaryVramAddr & 0xFF) | (u16) (0x3F & val) << 8;
                    } else {
                        temporaryVramAddr = (temporaryVramAddr & 0xFF00) | val;
                        vRamAddr = temporaryVramAddr;
                    }
                    addressLatch = !addressLatch;
                    return val;
                case 7:
                    ppuData = val;
                    ppu_write(vRamAddr & 0x3FFF, val);
                    vRamAddr += ppuCtl & 0x4 ? 32 : 1;
                    return val;
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
                num = ppuData;
                ppuData = ppu_read(vRamAddr & 0x3FFF);
                if ((vRamAddr & 0x3FFF) > 0x3EFF) {
                    return ppuData;
                }
                vRamAddr += ppuCtl & 0x4 ? 32 : 1;
                printf("reading from addr 0x%X chr-rom %X rendering is %d scanline is %d\n",
                       vRamAddr, num, rendering(), scanline);
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
        ppuData = 0;
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


