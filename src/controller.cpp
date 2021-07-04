//
// Created by Brian Bonafilia on 7/2/21.
//
#include <stdio.h>
#include <iostream>
#include "include/gui.hpp"

namespace Controller {

   GUI::controller_status controller1_status;
   bool strobe = false;
   int counter = 0;

   void setControllerStatus(bool setStrobe) {
       strobe = setStrobe;
       if (strobe == false) {
           controller1_status.state = GUI::getControllerStatus();
           counter = 8;
       }
   }

   u8 getController1() {
       if (counter == 0) {
           return 1;
       }
       u8 val = controller1_status.state & 1;
       controller1_status.state >>= 1;
       counter--;
//       std::cout << "this is state of controller " << std::bitset<8>(controller1_status.state) << std::endl;
//       if (counter == 0) {
//           printf("counter is 0, and val is %d\n", val);
//       }
       if (val == 1) {
           printf("returning val %d\n", val);
           printf("counter is %d\n", counter);
       }
       return val;
   }

   u8 getController2() {
       return 0;
   }

}

