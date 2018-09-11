#include <iostream>
#include <cstdlib>
#include <cstring>

#include "include/cpu.hpp"


namespace CPU {

  /* CPU state */

  u8 A, X ,Y, S;         //registers, these are as follows
  u16 PC;                  // A is Accumulator: supports carrying overflow
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

  /*if r is greater than 255 set Carry flag true
    if x + y creates overflow, i.e. x and y are same sign(+/-) and adding 
    creates opposite sign than they have overflowed */
  inline void upd_cv(u8 x, u8 y, u16 r) {
    P[C] = (r > 0xFF);
    P[V] = ~(x ^ y) & (x^r) & 0x80;
  }
  /*if x is negative set Negative flag true
    if x is 0 set Zero Flag true */
  inline void upd_nz(u8 x)  {
    P[N] = x & 0x80;
    P[Z] = (x == 0);
  }
  /* if a + i crosses a page(256 mem spots long) return true */
  inline bool cross(u16 a, u8 i){
    return ((a + i) & 0xFF00) != (a & 0xFF00);
  }
  /*memory access*/

  template <bool wr> u8 inline access(u16 addr, u8 v = 0){
    u8* r;

    switch(addr){
      
      /*RAM access or one of the 3 mirrors of RAM */ 
    case 0x0000 ... 0x1FFF: r = &ram[addr % 0x800]; if (wr) *r = v; return *r;

    case 0x2000 ... 0x3FFF:  /*TODO return PPU mem access registers*/ return 0;
      
    case 0x4000 ... 0x4017:  /*TODO APU and I/O registers*/ return 0;

    case 0x4020 ... 0xFFFF:   /*TODO Cartridge space: PRG ROM, PRG RAM, and
			       mapper registers */ return 0;
    }
    return 0;
  }

  /*ways to access memory*/
  //write to memory
  inline u8 wr(u16 a, u8 v)       { T; return access<true>(a,v); }
  //read from memory
  inline u8 rd(u16 a)             { T; return access<false>(a);  }
  //read from two addresses a,b,  and merge to 16 bit
  inline u16 rd16_d(u16 a, u16 b) {  return rd(a) | (rd(b) << 8); }
  //read two addys from a
  inline u16 rd16(u16 a)          {  return rd16_d(a, a+1); }
  //push value onto stack, and adjust stack pointer
  inline u8 push(u8 v)            {  return wr(0x100 + (S--), V); }
  //pop stack
  inline u8 pop()                 {  return rd(0x100 + (++S));    }

  /*Addressing Modes*/
  
  //immediate gets address  after OP code 
  inline u16 imm()          { return PC++;           }
  inline u16 imm16()        { PC += 2; return PC -2; }
  //read from address of 2 bytes after OP code
  inline u16 abs()          { return rd16(imm16());   }
  //read from address of 2 bytes and add to X
  inline u16 abx()          { u16 a = abs(); if(cross(a,X)) T; return a + X;}
  //same but for Y these absolute indexed modes
  inline u16 aby()          { u16 a = abs(); if(cross(a,Y)) T; return a + Y;}
  //read byte after OP call, zero page indexing
  inline u16 zp()           { return rd(imm()); }
  inline u16 zpx()          { u16 a = zp(); return (a + X) % 256; }
  inline u16 zpy()          { u16 a = zp(); return (a + Y) % 256; }

}
