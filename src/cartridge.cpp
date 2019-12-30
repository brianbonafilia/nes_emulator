#include <cstdio>
#include <cstdlib>
#include <iostream>
#include "include/cartridge.hpp"
#include "include/cpu.hpp"

namespace Cartridge
{

//Mapper will be include here to handle memory access

//access PRG ROM/RAM using mapper
template <bool wr>
u8 access(u16 addr, u8 v)
{
  //TODO mapper access
  return 0;
}
template <bool wr>
u8 chr_access(u16 addr, u8 v)
{
  //TODO mapper access
  return 0;
}

void load(const char *fileName)
{
  //Open to read binary file with ROM in it
  FILE *f = fopen(fileName, "rb");
  if (f == NULL)
  {
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
  if (result != size)
  {
    fputs("reading error", stderr);
    exit(2);
  }
  fclose(f);

  //Find Mapper
  u8 mapperID = (rom[7] & 0xF0) + (rom[6] >> 4);

  std::cout << (int) mapperID << std::endl;
  //
  

  //Start running the ROM file
  CPU::power();
  //TODO:  PPU start
}

bool loaded()
{
  //TODO: check if it is loaded into proper mapper
  return true;
}

} // namespace Cartridge
