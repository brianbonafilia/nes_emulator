//
// Created by Brian Bonafilia on 7/2/21.
//

#ifndef NES_EMULATOR_CONTROLLER_H
#define NES_EMULATOR_CONTROLLER_H

namespace Controller {

    void setControllerStatus(bool setStrobe);

    u8 getController1();

    u8 getController2();
}

#endif //NES_EMULATOR_CONTROLLER_H


