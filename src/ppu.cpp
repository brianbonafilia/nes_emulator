//
// Created by Brian Bonafilia on 6/1/21.
//

#include "include/SDL2/SDL.h"
#include <iostream>
#include "include/ppu.hpp"
#include "include/gui.hpp"

namespace PPU {
    /**
     * PPU control
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
     * PPU mask
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
     * PPU Status
     *
     * flags by bit VSO- ----
     *
     * V = vblank
     * S = sprite 0 hit
     * O = sprite overflow
     */
    u8 ppuStatus;

    /**
     * Object Attribute Memory(OAM) address
     *
     * the address to read/write to OAM
     */
    u8 oamAddr;

    /**
     * OAM data
     *
     * Write OAM data here
     */
    u8 oamData;

    /**
     * fine scroll position
     */
    u8 ppuScroll;

    /**
     * PPU Address
     *
     * ppu read/write address
     */
    u8 ppuAddr;

    /**
     * PPU Data
     *
     * ppu data read/write
     */
    u8 ppuData;

    /**
     * OAM Direct Memory Access
     *
     * Used to transfer large amounts of data to OAM
     */
    u8 oamDma;

    /**
     * This is to allow easy read write access
     */
    u8* registerAccess[8];

    u8 vRam[0x7FF];

    /**
     * Object Attribute Memory
     */
    u8 OAM[0xFF];

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

    u32 pixels[256*240];
    u32 sum = 0;

    u16 vRamAddr;
    u16 temporaryVramAddr;
    u8 fineXScroll;
    bool firstOrSecondWriteToggle;

    void doStep() {
        cycle++;
        if (scanline < 240  && cycle < 256) {
            u32 index = 240 * scanline + cycle;
            pixels[index] = ++sum;
        }
        if (cycle == 341) {
            cycle = 0;
            scanline++;
        }
        if (scanline == 260) {
            scanline = 0;
            GUI::update_frame(pixels);
        }

    }

    template<bool wr>
    u8 accessRegisters(u16 addr, u8 val) {
        u16 index = addr - 0x2000;
        u8* registerToAccess = registerAccess[index % 8];
        if (wr) {
            *registerToAccess = val;
       }
        return  *registerToAccess;
    }

    template u8 accessRegisters<true>(u16 addr, u8 val);
    template u8 accessRegisters<false>(u16 addr, u8 val);

    void power() {
        ppuCtl = 0;
        ppuMask = 0;
        ppuStatus = 0;
        oamAddr = 0;
        scanline = 0;

        scanline = 0;
        cycle = 0;

        registerAccess[0] = &ppuCtl;
        registerAccess[1] = &ppuMask;
        registerAccess[2] = &ppuStatus;
        registerAccess[3] = &oamAddr;
        registerAccess[4] = &oamData;
        registerAccess[5] = &ppuScroll;
        registerAccess[6] = &ppuAddr;
        registerAccess[7] = &ppuData;

        memset(vRam, 0xFF, sizeof(vRam));
        memset(OAM, 0xFF, sizeof(OAM));
    }
}


