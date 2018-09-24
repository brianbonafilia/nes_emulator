#pragma once
#include "common.hpp"

namespace Cartridge {
  
  //program ROM/RAM
  //bool wr determines whether we write or not
  template<bool wr> u8 access(u16 addr, u8 v = 0);
  //graphic ROM/RAM
  template<bool wr> u8 chr_access(u16 addr, u8 v = 0);

  //load the ROM from file into memory
  void load(const char* fileName);

  //return true if ROM has been loaded into memory
  bool loaded();

}
