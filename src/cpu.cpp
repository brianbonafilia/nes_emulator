#include <iostream>
#include <cstdlib>
#include <cstring>

#include "include/cpu.hpp"


namespace CPU {

  /* CPU state */

  u8 A, X ,Y, PC;         //registers, these are as follows
  u16 S;                  // A is Accumulator: supports carrying overflow
  Flags P;                // detection, and so on
                          // X and Y are used for addressing modes(indices)
                          // PC is program counter, S is Stack Pointer
                          // p is status register
  
  bool irq, nmi;          //irq is interrupt request
                          //nmi is non-maskable interrupt
  u8 ram[0x800];

  /* this keeps track of how many CPU cycles are done until next frame */
  const int TOTAL_CYCLES = 29781;
  int remainingCycles;
  inline int elapsed(){ return TOTAL_CYCLES - remainingCycles; }

  /*defining method tick to be T will make easier to include, in  many places
    tick will be called at the end of each operation */
  #define T tick()
  inline void tick() { /* TODO: add 3 PPU steps */ remainingCycles--;}



}
