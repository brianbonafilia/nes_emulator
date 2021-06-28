#include <iostream>
#include <cstdlib>
#include <cstring>
#include "unistd.h"

#include "include/cpu.hpp"
#include "include/cartridge.hpp"
#include "include/ppu.hpp"

namespace CPU {

/* CPU state */

    bool debug = false;
    bool test = false;

    int opCode;

    u8 A, X, Y, S; //registers, these are as follows
    u16 PC;        // A is Accumulator: supports carrying overflow
    Flags P;       // detection, and so on
    // X and Y are used for addressing modes(indices)
    // PC is program counter, S is Stack Pointer
    // p is status register

    bool irq, nmi; //irq is interrupt request
    //nmi is non-maskable interrupt
    u8 ram[0x800];

/* this keeps track of how many CPU cycles are done until next frame */
    const int TOTAL_CYCLES = 29781;
    int remainingCycles;

    inline int elapsed() { return TOTAL_CYCLES - remainingCycles; }

/*defining method tick to be T will make easier to include, in  many places
    tick will be called during each operation */
#define T tick()

    inline void tick() {
        remainingCycles--;
        PPU::doStep();
        PPU::doStep();
        PPU::doStep();
    }

/*if r is greater than 255 set Carry flag true
    if x + y creates overflow, i.e. x and y are same sign(+/-) and adding 
    creates opposite sign than they have overflowed */
    inline void upd_cv(u8 x, u8 y, u16 r) {
        P[C] = (r > 0xFF);
        P[V] = ~(x ^ y) & (x ^ r) & 0x80;
    }

    void set_debug(bool val) {
        debug = val;
    }

/*if x is negative set Negative flag true
    if x is 0 set Zero Flag true */
    inline void upd_nz(u8 x) {
        P[N] = x & 0x80;
        P[Z] = (x == 0);
    }

/* if a + i crosses a page(256 mem spots long) return true */
    inline bool cross(u16 a, u8 i) {
        return ((a + i) & 0xFF00) != (a & 0xFF00);
    }

/*memory access*/

    template<bool wr>
    u8 inline access(u16 addr, u8 v = 0) {
        u8 *r;

        switch (addr) {

            /*RAM access or one of the 3 mirrors of RAM */
            case 0x0000 ... 0x1FFF:
                r = &ram[addr % 0x800];
                if (wr)
                    *r = v;
                return *r;

            case 0x2000 ... 0x3FFF: /*TODO return PPU mem access registers*/
                //std::cout << "PC : " << std::to_string(PC) << std::endl;
                //std::cout << "addr : " << std::to_string(addr) << std::endl;
                return PPU::accessRegisters<wr>(addr, v);

            case 0x4000 ... 0x4017: /*TODO APU and I/O registers*/
                return 0;

            case 0x4020 ... 0xFFFF: /*TODO Cartridge space: PRG ROM, PRG RAM, and
			       mapper registers */
                return Cartridge::access<wr>(addr, v);
        }
        return 0;
    }

/*ways to access memory*/
//write to memory
    inline u8 wr(u16 a, u8 v) {
        T;
        return access<true>(a, v);
    }

//read from memory
    inline u8 rd(u16 a) {
        T;
        return access<false>(a);
    }

//read from two addresses a,b,  and merge to 16 bit
    inline u16 rd16_d(u16 a, u16 b) { return rd(a) | (rd(b) << 8); }

//read two addys from a
    inline u16 rd16(u16 a) { return rd16_d(a, a + 1); }

//push value onto stack, and adjust stack pointer
    inline u8 push(u8 v) { return wr(0x100 + (S--), v); }

//pop stack
    inline u8 pop() { return rd(0x100 + (++S)); }

/*Addressing Modes*/

//immediate gets address  after OP code
    inline u16 imm() { return PC++; }

    inline u16 imm16() {
        PC += 2;
        return PC - 2;
    }

//read from address of 2 bytes after OP code
    inline u16 abs() { return rd16(imm16()); }

//read from address of 2 bytes and add to X
    inline u16 abx() {
        u16 a = abs();
        if (cross(a, X))
            T;
        return a + X;
    }

//Special case,  Tick regardless of page cross as is write to memory
    inline u16 _abx() {
        T;
        return abs() + X;
    }

//same but for Y these absolute indexed modes
    inline u16 aby() {
        u16 a = abs();
        if (cross(a, Y))
            T;
        return a + Y;
    }

//read byte after OP call, zero page indexing
    inline u16 zp() { return rd(imm()); }

    inline u16 zpx() {
        u16 a = zp();
        return (a + X) % 256;
    }

    inline u16 zpy() {
        u16 a = zp();
        return (a + Y) % 256;
    }

//indirect addressing
    inline u16 izx() {
        u8 i = zpx();
        return rd16_d(i, (i + 1) % 0x100);
    }

    inline u16 _izy() {
        u8 i = zp();
        return rd16_d(i, (i + 1) % 0x100) + Y;
    }

    inline u16 izy() {
        u16 a = _izy();
        if (cross(a - Y, Y))
            T;
        return a;
    }

//Load accumulator OPs
    template<Mode m>
    void LDA() {
        u16 a = m();
     //   printf("    a is %x    ", a);
        u8 t = rd(a);
       // printf("    new val for A is %x   ", t);
        upd_nz(t);
        A = t;
    }

//Load X register
    template<Mode m>
    void LDX() {
        u16 a = m();
        //printf(" a is $%02X",a);
        u8 t = rd(a);
        //printf(" t is %d   ", t);
        upd_nz(t);
        X = t;
    }

//Load Y register
    template<Mode m>
    void LDY() {
        u16 a = m();
        u8 t = rd(a);
        upd_nz(t);
        Y = t;
    }

/*STx ops */
    template<u8 &r, Mode m>
    void st() { wr(m(), r); }

    template<>
    void st<A, abx>() {
        T;
        wr(abs() + X, A);
    }

    template<>
    void st<A, aby>() {
        T;
        wr(abs() + Y, A);
    }

    template<>
    void st<A, izy>() {
        T;
        wr(_izy(), A);
    }

/*Transfer OPS*/
    template<u8 &d, u8 &s>
    void tr() {
        upd_nz(s = d);
        T;
    }

    template<>
    void tr<X, S>() {
        S = X;
        T;
    }
//no need to update flags for TXS ^^

/*get value at address using address mode */
#define G      \
  u16 a = m(); \
  u8 p = rd(a);

/*ADC*/
    template<Mode m>
    void ADC() {
        G;
        u16 r = A + p + P[C];
        upd_cv(A, p, r);
        upd_nz(A = r);
    }
/*SBC*/
//Subtract from accumulator
    template<Mode m>
    void SBC() {
        G;
        p = ~p; //take complement of value taken from memory
        u16 r = A + p + P[C];
        upd_cv(A, p, r);
        upd_nz(A = r);
    }

/*DEC from memory-- */
    template<Mode m>
    void DEC() {
        G;
        T;
        wr(a, --p);
        upd_nz(p);
    }

/* decrement from registers */
    void DEX() {
        T;
        upd_nz(--X);
    }

    void DEY() {
        T;
        upd_nz(--Y);
    }

/*INC from memory*/
    template<Mode m>
    void INC() {
        G;
        T;
        wr(a, ++p);
        upd_nz(p);
    }

/*increment registers*/
    void INX() {
        T;
        upd_nz(++X);
    }

    void INY() {
        T;
        upd_nz(++Y);
    }

/*BITWISE OPS*/

    template<Mode m>
    void AND() {
        G;
        u8 v = A & p;
        upd_nz(A = v);
    }

//Shift left 1 bit, accumulater
    void ASL() {
        u16 r = A << 1;
        P[C] = r > 0xFF;
        upd_nz(A = r);
        T;
    }

//shift memory location left
    template<Mode m>
    void ASL() {
        G;
        P[C] = p & 0x80; //shift leftmost bit into carry flag;
        T;
        upd_nz(wr(a, p << 1));
    }

/*BIT testing, bits 6 and 7 go status register(N and V) */
    template<Mode m>
    void BIT() {
        G;
        P[Z] = !(A & p);
        P[N] = p & 0x80; //bit 7 to N
        P[V] = p & 0x40; //bit 6 to V
    }

//exclusive OR
    template<Mode m>
    void EOR() {
        G;
        upd_nz(A = (p ^ A));
    }

//Shift one bit right move, move 0th bit to Carry
    void LSR() {
        P[C] = A & 0x01;
        upd_nz(A >>= 1);
        T;
    }

    //Shift left one bit, then or with memory
    template<Mode m>
    void SLO() {
        G;
        P[C] = p & 0xF0;
        upd_nz(wr(a, p << 1));
    }

    template<Mode m>
    void LSR() {
        G;
        P[C] = p & 0x01;
        upd_nz(wr(a, p >> 1));
        T;
    }

//Or value from memory with Accumulator, insert result into A
    template<Mode m>
    void ORA() {
        G;
        upd_nz(A |= p);
    }

//Rotate value one bit to left, update carry with MSB, update N,Z
    template<Mode m>
    void ROL() {
        G;
        T;
        P[C] = p & 0x80;
        p = (p << 1) + P[C];
        upd_nz(wr(a, p));
    }

    void ROL() {
        P[C] = A & 0x80;
        A = (A << 1) + P[C];
        upd_nz(A);
        T;
    }

//Rotate one bit right, update carry with LSB, update N,Z
    template<Mode m>
    void ROR() {
        G;
        u8 carry = P[C];
        P[C] = p & 0x01;
        p = (p >> 1) + (carry << 7);
        upd_nz(wr(a, p));
        T;
    }

    void ROR() {
        u8 carry = A & 0x1;
        A = (A >> 1) + (P[C] << 7);
        P[C] = carry;
        upd_nz(A);
        T;
    }

//Branch on carry clear, P[C] = 0, PC will move to next location
    void BCC() {
        s8 p = rd(imm());
        if (!P[C]) {
            T;
            if (cross(PC, p))
                T;
            PC += p;
        }
    }

//Branch on carry set, if P[C], program counter will move to next location
    void BCS() {
        s8 p = rd(imm());
        if (P[C]) {
            T;
            if (cross(PC, p))
                T;
            PC += p;
        }
    }

//branch on result zero
    void BEQ() {
        s8 p = rd(imm());
        if (P[Z]) {
            T;
            if (cross(PC, p))
                T;
            PC += p;
        }
    }

//branch on result minus
    void BMI() {
        s8 p = rd(imm());
        if (P[N]) {
            T;
            if (cross(PC, p))
                T;
            PC += p;
        }
    }

//branch on not zero
    void BNE() {
        s8 p = rd(imm());
        if (!P[Z]) {
            T;
            if (cross(PC, p))
                T;
            PC += p;
        }
    }

//branch on result plus
    void BPL() {
        s8 p = rd(imm());
        if (!P[N]) {
            T;
            if (cross(PC, p))
                T;
            PC += p;
        }
    }

//Branch on overflow flag clear
    void BVC() {
        s8 p = rd(imm());
        if (!P[V]) {
            T;
            if (cross(PC, p))
                T;
            PC += p;
        }
    }

//Branch on carry flag set
    void BVS() {
        s8 p = rd(imm());
        if (P[V]) {
            T;
            if (cross(PC, p))
                T;
            PC += p;
        }
    }
/*Stack Operations*/
//Push A onto stack
    void PHA() {
        T;
        push(A);
    }

//Pull A from stack
    void PLA() {
        T;
        T;
        A = pop();
        upd_nz(A);
    }

//Push Processor Statuses onto stack
    void PHP() {
        T;
        push(P.get() | (1 << 4));
    } //set B flag
//Pull Processor Status from stack
    void PLP() {
        T;
        T;
        u8 temp = pop();
        //std::cout << " putting into P = " << (int) temp << std::endl;
        P.set(temp);
    }
//

//Jump to address made from next 2 bytes
    void JMP() {
        PC = abs();
    }
/*Indirect JMP */
//Jump to address, by reading from memory at address of next 2 bytes
    void i_JMP() {
        u16 a = abs();
        if (cross(a, 1))
            PC = rd16_d(a, a - 0xFF);
        else
            PC = rd16(a);
    }

//Jump to subroutine
    void JSR() {
        u16 t = PC + 1;
        T;
        push(t >> 8);
        push(t);
        PC = rd16(imm16());
    }

//Return from interrupt
    void RTI() {
        T;
        T;
        P.set(pop());
        PC = pop();
        PC = pop() << 8 | PC;
    }

//Return from subroutine
    void RTS() {
        T;
        T;
        PC = pop() | pop() << 8;
        PC++;

        //std::cout << "jumpoint to sub from " << std::hex << (int) PC << std::endl;
        T;
    }

//BReaK
    void BRK() {
        T;
        u16 t = PC + 2;
        push(t >> 8);
        push(t);
        PC = rd(0xFFFE);
        PC = (rd(0xFFFF) << 8) | PC;
        push(P.get() | (1 << 4));
    }

/*Status Register Change*/
//Clear flag
    template<Flag f>
    void cl() {
        P[f] = 0;
        T;
    }

//Set flag
    template<Flag f>
    void set() {
        P[f] = 1;
        T;
    }

/*Compare Ops  CMx*/
//Register - Memory
// Memory > Register : set N
// Memory = Register : set Z and C
// Memory < Register : set C
    template<u8 &r, Mode m>
    void cmp() {
        G;
        upd_nz(r - p);
        P[C] = (r >= p);
    }

    // Double No op
    template<Mode m>
    void DOP() {
        m();
        T;
        T;
    }

    // Triple No op
    void TOP() {
        T;T;T;
    }

    // increase memory by one
    template <Mode m>
    void ISC() {
        G;
        p++;
        upd_cv(A, -p, a);
        wr(a, p);
        A -= p + P[C];
    }

    // Shift right one bit then EOR accumulator with memory
    template <Mode m>
    void SRE() {

    }

    template <Mode m>
    void DCP() {
        G;
        wr(a, p--);
    }

    void NOP() { T; }

    void exec() {
        if (debug) {
            sleep(1);
            std::cout << " Program Counter " << std::hex << PC % 0x8000;
            std::cout << " performing OP code " << std::hex << (int) rd(PC);
            std::cout << " A = " << (int) A << " X = " << (int) X << " Y = " << (int) Y;
            std::cout << " P =  " << std::hex << (int) P.get();
            std::cout << " S = " << std::hex << (int) S << std::endl;
        } if (test) {
            printf("%04X ", PC);
//            std::cout << " " << std::hex << (int) rd(PC);
//            std::cout << " A:" << (int) A << " X:" << (int) X << " Y:" << (int) Y;
            printf("%02X A:%02X X:%02X Y:%02X P:%02X SP:%02X \n",rd(PC), A, X, Y, P.get(), S);
//            std::cout << " CYC:" << std::to_string(PPU::getCycle());
//            std::cout << " SL:" << std::to_string(PPU::getScanline()) << std::endl;
        }
        opCode = rd(PC++);
        switch (opCode) {

            case 0x03:
                return SLO<izx>();
            case 0x14:
                return DOP<zpx>();
            case 0x1A:
                return NOP();
            case 0x1C:
                return TOP();
            /*Storage OPs */
            //LDA
            case 0xA9:
                return LDA<imm>();
            case 0xA5:
                return LDA<zp>();
            case 0xB5:
                return LDA<zpx>();
            case 0xAD:
                return LDA<abs>();
            case 0xBD:
                return LDA<abx>();
            case 0xB9:
                return LDA<aby>();
            case 0xA1:
                return LDA<izx>();
            case 0xB1:
                return LDA<izy>();
                //LDX
            case 0xA2:
                return LDX<imm>();
            case 0xA6:
                return LDX<zp>();
            case 0xB6:
                return LDX<zpy>();
            case 0xAE:
                return LDX<abs>();
            case 0xBE:
                return LDX<aby>();
                //LDY
            case 0xA0:
                return LDY<imm>();
            case 0xA4:
                return LDY<zp>();
            case 0xB4:
                return LDY<zpx>();
            case 0xAC:
                return LDY<abs>();
            case 0xBC:
                return LDY<abx>();

                //STA
            case 0x85:
                return st<A, zp>();
            case 0x95:
                return st<A, zpx>();
            case 0x8D:
                return st<A, abs>();
            case 0x9D:
                return st<A, abx>();
            case 0x99:
                return st<A, aby>();
            case 0x81:
                return st<A, izx>();
            case 0x91:
                return st<A, izy>();

                //STX
            case 0x86:
                return st<X, zp>();
            case 0x96:
                return st<X, zpy>();
            case 0x8E:
                return st<X, abs>();

                //STY
            case 0x84:
                return st<Y, zp>();
            case 0x94:
                return st<Y, zpx>();
            case 0x8C:
                return st<Y, abs>();

                //TAY
            case 0xA8:
                return tr<A, Y>();

                //TAX
            case 0xAA:
                return tr<A, X>();

                //TSX
            case 0xBA:
                return tr<S, X>();

                //TXA
            case 0x8A:
                return tr<X, A>();

                //TXS
            case 0x9A:
                return tr<X, S>();

                //TYA
            case 0x98:
                return tr<Y, A>();

                //ADC
            case 0x69:
                return ADC<imm>();
            case 0x65:
                return ADC<zp>();
            case 0x75:
                return ADC<zpx>();
            case 0x6D:
                return ADC<abs>();
            case 0x7D:
                return ADC<abx>();
            case 0x79:
                return ADC<aby>();
            case 0x61:
                return ADC<izx>();
            case 0x71:
                return ADC<izy>();

                //SBC
            case 0xE9:
                return SBC<imm>();
            case 0xE5:
                return SBC<zp>();
            case 0xF5:
                return SBC<zpx>();
            case 0xED:
                return SBC<abs>();
            case 0xFD:
                return SBC<abx>();
            case 0xF9:
                return SBC<aby>();
            case 0xE1:
                return SBC<izx>();
            case 0xF1:
                return SBC<izy>();

                //DEC
            case 0xC6:
                return DEC<zp>();
            case 0xD6:
                return DEC<zpx>();
            case 0xCE:
                return DEC<abs>();
            case 0xDE:
                return DEC<_abx>(); // use _abx because we always Tick to check
                // if writing to right mem location page
                // cross

                //DEX
            case 0xCA:
                return DEX();

                //DEY
            case 0x88:
                return DEY();

                //INC
            case 0xE6:
                return INC<zp>();
            case 0xF6:
                return INC<zpx>();
            case 0xEE:
                return INC<abs>();
            case 0xFE:
                return INC<_abx>(); //Tick regardless of page cross

                //INX
            case 0xE8:
                return INX();

                //INY
            case 0xC8:
                return INY();

                //AND
            case 0x29:
                return AND<imm>();
            case 0x25:
                return AND<zp>();
            case 0x35:
                return AND<zpx>();
            case 0x2D:
                return AND<abs>();
            case 0x3D:
                return AND<abx>();
            case 0x39:
                return AND<aby>();
            case 0x21:
                return AND<izx>();
            case 0x31:
                return AND<_izy>(); //

                //ASL
            case 0x0A:
                return ASL();
            case 0x06:
                return ASL<zp>();
            case 0x16:
                return ASL<zpx>();
            case 0x0E:
                return ASL<abs>();
            case 0x1E:
                return ASL<_abx>(); //Always tick when writing to mem (x page)

                //BIT
            case 0x24:
                return BIT<zp>();
            case 0x2C:
                return BIT<abs>();

                //EOR
            case 0x49:
                return EOR<imm>();
            case 0x45:
                return EOR<zp>();
            case 0x55:
                return EOR<zpx>();
            case 0x4D:
                return EOR<abs>();
            case 0x5D:
                return EOR<abx>();
            case 0x59:
                return EOR<aby>();
            case 0x41:
                return EOR<izx>();
            case 0x51:
                return EOR<izy>();

                //LSR
            case 0x4A:
                return LSR();
            case 0x46:
                return LSR<zp>();
            case 0x56:
                return LSR<zpx>();
            case 0x4E:
                return LSR<abs>();
            case 0x5E:
                return LSR<_abx>();

                //ORA
            case 0x09:
                return ORA<imm>();
            case 0x05:
                return ORA<zp>();
            case 0x15:
                return ORA<zpx>();
            case 0x0D:
                return ORA<abs>();
            case 0x1D:
                return ORA<abx>();
            case 0x19:
                return ORA<aby>();
            case 0x01:
                return ORA<izx>();
            case 0x11:
                return ORA<_izy>();

                //ROL
            case 0x2A:
                return ROL();
            case 0x26:
                return ROL<zp>();
            case 0x36:
                return ROL<zpx>();
            case 0x2E:
                return ROL<abs>();
            case 0x3E:
                return ROL<_abx>();

                //ROR
            case 0x6A:
                return ROR();
            case 0x66:
                return ROR<zp>();
            case 0x76:
                return ROR<zpx>();
            case 0x6E:
                return ROR<abs>();
            case 0x7E:
                return ROR<_abx>();

                /*Stack Operations */
            case 0x48:
                return PHA();
            case 0x08:
                return PHP();
            case 0x68:
                return PLA();
            case 0x28:
                return PLP();

                //BRANCH
            case 0x90:
                return BCC();
            case 0xB0:
                return BCS();
            case 0xF0:
                return BEQ();
            case 0x30:
                return BMI();
            case 0xD0:
                return BNE();
            case 0x10:
                return BPL();
            case 0x50:
                return BVC();
            case 0x70:
                return BVS();

                //JMP
            case 0x4C:
                return JMP();
            case 0x6C:
                return i_JMP();
            case 0x20:
                return JSR();
            case 0x40:
                return RTI();
            case 0x60:
                return RTS();
            case 0x00:
                return BRK();

                //Flag Setting and Clearing
            case 0x18:
                return cl<C>(); //clear
            case 0xD8:
                return cl<D>();
            case 0x58:
                return cl<I>();
            case 0xB8:
                return cl<V>();
            case 0x38:
                return set<C>(); //set
            case 0xF8:
                return set<D>();
            case 0x78:
                return set<I>();

                //Compare OPS
                //Compare against A (CMP)
            case 0xC9:
                return cmp<A, imm>();
            case 0xC5:
                return cmp<A, zp>();
            case 0xD5:
                return cmp<A, zpx>();
            case 0xCD:
                return cmp<A, abs>();
            case 0xDD:
                return cmp<A, abx>();
            case 0xD9:
                return cmp<A, aby>();
            case 0xC1:
                return cmp<A, izx>();
            case 0xD1:
                return cmp<A, izy>();

                //Compare X (CPX)
            case 0xE0:
                return cmp<X, imm>();
            case 0xE4:
                return cmp<X, zp>();
            case 0xEC:
                return cmp<X, abs>();

                //Compare Y (CPY)
            case 0xC0:
                return cmp<Y, imm>();
            case 0xC4:
                return cmp<Y, zp>();
            case 0xCC:
                return cmp<Y, abs>();

                //NOP
            case 0xEA:
                return NOP();

                //Unofficial
            case 0x04:
                PC++;
                return NOP();
            case 0xFF:
                //return exit(1);
                exit(0);
                //return ISC<abx>();
            case 0xCF:
                return DCP<abs>();
            case 0xD3:
                return DCP<izy>();
            case 0xD7:
                 return DCP<zpx>();
            case 0xDB:
                return DCP<aby>();
            case 0xDF:
                return DCP<abx>();
            case 0xC7:
                return DCP<zp>();

            case 0xD2:
                exit(1);


            default:
                NOP();
                if (debug == false) {
                    std::cout << "undefined op" << std::endl;
                    std::cout << "Op code - " << std::hex << (int) rd(PC - 1) << std::endl;
                    //std::cout << "program counter value = " << (int) PC << std::endl;
                }
        }
    }

    void set_nmi(bool v) { nmi = v; }

    void set_irq(bool v) { irq = v; }

//reset interrupt
    void reset() {
        S -= 3;
        set_irq();
        P[I] = 1;
        T;
        T;
        T;
        T;
        T;
        PC = rd16(0xFFFC);
        //PC = 0xC000;
    }

//regular interrupt request
    void irq_interrupt() {
        T;
        T;
        push(PC >> 8);
        push(PC & 0xFF);
        push(P.get());
        PC = rd16(0xFFFE);
    }

    //non maskable interrupt
    void nmi_interrupt() {
        T;
        T;
        push(PC >> 8);
        push(PC);
        push(P.get());
        PC = rd16(0xFFFA);
        nmi = false;
    }

//set up CPU state on start
    void power() {
        A = 0;
        X = 0;
        Y = 0;
        P.set(0x34);
        remainingCycles = 0;
        S = 0x00; //When reset is done sets to 0xFD like expected
        memset(ram, 0xFF, sizeof(ram));

        nmi = false;
        irq = false;
        //reset

        reset();
    }

    void run_frame() {

        remainingCycles += TOTAL_CYCLES;

        while (remainingCycles > 0) {
            /*interrupt */
            if (nmi) {
                nmi_interrupt();
            }
                /*other interrupt: also do stuff */
            else if (irq and !P[I]) {
                irq_interrupt();
            }
            exec();
        }
        //TODO frame elapsed do the stuff
    }
} // namespace CPU
