#include "include/cpu.hpp"
#include "include/cartridge.hpp"


int main(int argc, char *argv[]) {
    Cartridge::load("nestest.nes");
    while (true) {
        CPU::run_frame();
    }
}