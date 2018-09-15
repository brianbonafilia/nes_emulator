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
  //Special case?  Tick regardless? will look into
  inline u16 _abx()         { T; return abs() + X;}
  //same but for Y these absolute indexed modes
  inline u16 aby()          { u16 a = abs(); if(cross(a,Y)) T; return a + Y;}
  //read byte after OP call, zero page indexing
  inline u16 zp()           { return rd(imm()); }
  inline u16 zpx()          { u16 a = zp(); return (a + X) % 256; }
  inline u16 zpy()          { u16 a = zp(); return (a + Y) % 256; }
  //indirect addressing 
  inline u16 izx()          { u8 i = zpx(); return rd16_d(i, (i+1) % 0x100); }
  inline u16 _izy()         { u8 i = zp(); return rd16_d(i,(i+1) % 0x100) + Y;}
  inline u16 izy()          { u8 a = _izy(); if(cross(a-Y, Y)) T; return a; }
  
  //Load accumulator OPs
  template<Mode m> void LDA(){
    u16 a = m();
    u8 t = rd(a);
    upd_nz(t);
    A = t;
  }

  //Load X register
  template<Mode m> void LDX(){
    u16 a = m();
    u8 t = rd(a);
    upd_nz(t);
    X = t;
  }

  //Load Y register
  template<Mode m> void LDY(){
    u16 a = m();
    u8 t = rd(a);
    upd_nz(t);
    Y = t;
  }

  /*STx ops */
  template<u8& r, Mode m> void st() { wr( m(), r); }
  template<>              void st<A, abx>() { T; wr( abs() + X, A); } 
  template<>              void st<A, aby>() { T; wr( abs() + Y, A); }
  template<>              void st<A, izy>() { T; wr(_izy(), A); }

  /*Transfer OPS*/
  template<u8& d, u8& s> void tr(){  upd_nz(s=d);  T; }
  template<>           void tr<X,S>() { S = X;   T;      }  
  //no need to update flags for TXS ^^

  /*get value at address gotten using address mode */
#define G u16 a = m(); u8 p = rd(a);

  /*ADC*/
  template<Mode m> void ADC() {   
    G;     
    u16 r = A + p + P[C];
    upd_cv(A,p,r);
    upd_nz(A = r);
  }
  /*SBC*/
  //Subtract from accumulator
  template<Mode m> void SBC()  {
    G;
    p = ~p;  //take complement of value taken from memory
    u16 r = A + p + P[C];
    upd_cv(A,p, r);
    upd_nz(A = r);
  }

  /*DEC from memory-- */
  template<Mode m> void DEC(){
    G;
    T;
    wr(a,--p);
    upd_nz(p);
  }
  /* decrement from registers */
  void DEX(){  T;  upd_nz(--X); }
  void DEY(){  T;  upd_nz(--Y); }

  /*INC from memory*/
  template<Mode m> void INC(){
    G;
    T;
    wr(a,++p);
    upd_nz(p);
  }

  /*increment registers*/
  void INX(){ T; upd_nz(++X); }
  void INY(){ T; upd_nz(++Y); }
 
  /*BITWISE OPS*/

  template<Mode m> void AND(){
    G;
    u8 v = A & p;
    upd_nz(A = v);
  }

  //Shift left 1 bit, accumulater
  void ASL(){ u16 r = A << 1; P[C] = r > 0xFF; upd_nz(A = r); T; }

  //shift memory location left
  template<Mode m> void ASL(){
    G;
    P[C] = p & 0x80;   //shift leftmost bit into carry flag;
    T;
    upd_nz(wr(a, p << 1));
  } 

  void NOP()         { T; }

  void exec(){
    switch(rd(PC++)){
    case 0x00:   return; //BRK  TODO Interrup Stuff 
      /*Storage OPs */
      //LDA
    case 0xA9: return LDA<imm>();
    case 0xA5: return LDA<zp>();
    case 0xB5: return LDA<zpx>();
    case 0xAD: return LDA<abs>();
    case 0xBD: return LDA<abx>();
    case 0xB9: return LDA<aby>();
    case 0xA1: return LDA<izx>();
    case 0xB1: return LDA<izy>();
      //LDX
    case 0xA2: return LDX<imm>();
    case 0xA6: return LDX<zp>();
    case 0xB6: return LDX<zpx>();
    case 0xAE: return LDX<abs>();
    case 0xBE: return LDX<abx>();
      //LDY
    case 0xA0: return LDY<imm>();
    case 0xA4: return LDY<zp>();
    case 0xB4: return LDY<zpx>();
    case 0xAC: return LDY<abs>();
    case 0xBC: return LDY<abx>();
      
      //STA
    case 0x85: return st<A,zp>();
    case 0x95: return st<A,zpx>();
    case 0x8D: return st<A,abs>();
    case 0x9D: return st<A,abx>();
    case 0x99: return st<A,aby>();
    case 0x81: return st<A,izx>();
    case 0x91: return st<A,izy>();

      //STX
    case 0x86: return st<X,zp>();
    case 0x96: return st<X,zpy>();
    case 0x8E: return st<X,abs>();

      //STY
    case 0x84: return st<Y,zp>();
    case 0x94: return st<Y,zpx>();
    case 0x8C: return st<Y,abs>();

      //TAY
    case 0xA8: return tr<A,Y>();

      //TAX
    case 0xAA: return tr<A,X>();

      //TSX
    case 0xBA: return tr<S,X>();

      //TXA
    case 0x8A: return tr<X,A>();

      //TXS
    case 0x9A: return tr<X,S>();

      //TYA
    case 0x98: return tr<Y,A>();

      //ADC
    case 0x69: return ADC<imm>();
    case 0x65: return ADC<zp>();
    case 0x75: return ADC<zpx>();
    case 0x60: return ADC<abs>();
    case 0x70: return ADC<abx>();
    case 0x79: return ADC<aby>();
    case 0x61: return ADC<izx>();
    case 0x71: return ADC<izy>();

      //SBC
    case 0xE9: return SBC<imm>();
    case 0xE5: return SBC<zp>();
    case 0xF5: return SBC<zpx>();
    case 0xED: return SBC<abs>();
    case 0xFD: return SBC<abx>();
    case 0xF9: return SBC<aby>();
    case 0xE1: return SBC<izx>();
    case 0xF1: return SBC<izy>();

      //DEC
    case 0xC6: return DEC<zp>();
    case 0xD6: return DEC<zpx>();
    case 0xCE: return DEC<abs>();
    case 0xDE: return DEC<_abx>(); // use _abx because we always Tick to check
                                   // if writing to right mem location page
                                   // cross

      //DEX
    case 0xCA: return DEX();

      //DEY
    case 0x88: return DEY();

      //INC
    case 0xE6: return INC<zp>();
    case 0xF6: return INC<zpx>();
    case 0xEE: return INC<abs>();  
    case 0xFE: return INC<_abx>();  //Tick regardless of page cross

      //INX
    case 0xE8: return INX();

      //INY
    case 0xC8: return INY();

      //AND
    case 0x29: return AND<imm>();
    case 0x25: return AND<zp>();
    case 0x35: return AND<zpx>();
    case 0x2D: return AND<abs>();
    case 0x3D: return AND<abx>();
    case 0x39: return AND<aby>();
    case 0x21: return AND<izx>();
    case 0x31: return AND<izy>();

      //ASL
      //    case
    }
  }
  
  void run_frame(){
    
    remainingCycles += TOTAL_CYCLES;

    while( remainingCycles > 0){
      /*interrupt: do something */
      if(nmi)
	;
      /*other interrupt: also do stuff */
      else if(irq and !P[I])
	;
      
      exec();
    }

    //TODO frame elapsed do the stuff
  }
}
