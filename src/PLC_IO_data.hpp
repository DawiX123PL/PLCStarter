#pragma once

#include "inttypes.h"



struct PLC_io_module{

    uint32_t input;
    uint32_t output;

};


struct PLC_io_tag{

    enum class Type{
        _float,
        _i32,
        _bool,
    } type;

    union Data
    {
        float _float;
        int32_t _i32;
        bool _bool;
    } data;


};






