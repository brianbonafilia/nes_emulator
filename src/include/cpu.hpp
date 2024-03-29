#pragma once

#include "common.hpp"


namespace CPU {

    void set_debug(bool debug = true);

    /*   Processor Flags
    /    C represents Carry Flag, is used also in shift and rotate ops
    /    Z represents Zero Flag, is set to 1 when any arithmetic or logical
    /      op produces a zero result. and 0 otherwise
    /    I represents interrupt, if set to 1 interrupts are disabled, other
    /      wise they are enabled
    /    D is decimal mode status flag.
    /    V is Overflow flag, and is set when arithmetic op produces too large
    /      to be represented in a byte
    /    N Sign flag: this flag is set if the result of an operation is negative
    /      cleared if positive
    */
    enum Flag {
        C, Z, I, D, V, N
    };

    //addressing mode
    typedef u16 (*Mode)(void);

    class Flags {

        bool f[6];  //represent each of 6 enum Flags

    public:

        //allows for accessing flag based on indexed enum
        bool &operator[](const int i) { return f[i]; }

        /*get turns boolean array into value which would be found in register */
        u8 get() {
            return (f[C] | f[Z] << 1 | f[I] << 2 | f[D] << 3 | 1 << 5 |
                    f[V] << 6 | f[N] << 7);
        }

        /* takes value that would be in register and sets bool array */
        void set(u8 p) {
            f[C] = NTH_BIT(p, 0);
            f[Z] = NTH_BIT(p, 1);
            f[I] = NTH_BIT(p, 2);
            f[D] = NTH_BIT(p, 3);
            f[V] = NTH_BIT(p, 6);
            f[N] = NTH_BIT(p, 7);
        }

    };

    void set_nmi(bool v = true);

    void set_irq(bool v = true);

    void power();

    void run_frame();
}
