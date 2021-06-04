#include <iostream>
#include "unistd.h"

#include "include/gui.hpp"
#include "include/cartridge.hpp"

int main(int argc, char *argv[]) {
    std::cout << "the ROM we are using is " << argv[1] << std::endl;
    Cartridge::load(argv[1]);
    return GUI::init();
}