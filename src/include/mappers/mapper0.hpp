#pragma once
#include "../mapper.hpp"

class Mapper0 : public Mapper {
    public:
        Mapper0(u8 *rom) : Mapper::Mapper(rom){

        }
};