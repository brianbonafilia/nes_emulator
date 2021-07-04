#pragma once

#include "common.hpp"

namespace GUI {

    struct ControllerState {
        unsigned A: 1;
        unsigned B: 1;
        unsigned select: 1;
        unsigned start: 1;
        unsigned up: 1;
        unsigned down: 1;
        unsigned left: 1;
        unsigned right: 1;
    };

    typedef union {
        u8 state;
        struct ControllerState controllerState;
    } controller_status;

    u8 getControllerStatus();

    int init();

    void update_frame(u32* pixels);
}
