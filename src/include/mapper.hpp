#pragma once
#include <cstring>
#include "common.hpp"

/*This class will be parent to other Mapper classes */

class Mapper
{

  u8 *rom;             //array of ROM
  bool chrRam = false; //we assume chrRom by default

protected:
  u32 prgMap[4]; //I think this has to do with bank switching
  u32 chrMap[4]; //or registers in the mapper

  u8 *prg, *chr, *prgRam;           //prg-ROM, chr-ROM, and prgRAM
  u32 prgSize, chrSize, prgRamSize; //size of the above arrays

  template <int pageKBs>
  void map_prg(int slot, int bank);
  template <int pageKBs>
  void map_chr(int slot, int bank);

public:
  Mapper(u8 *rom);
  ~Mapper();

  u8 read(u16 addr);
  virtual u8 write(u16 addr, u8 val) { return val; }

  u8 chr_read(u16 addr);
  virtual u8 chr_write(u16 addr, u8 v) { return v; }

  virtual void signal_scanline() {}
};
