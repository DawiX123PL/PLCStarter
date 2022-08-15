#pragma once
#include <inttypes.h>
#include <string>
#include <sstream>
#include <iomanip>


struct alignas(uint32_t) PLC_IO_Module_Data
{
    uint32_t input = 0;
    uint32_t output = 0;
    struct Flags
    {
        bool error : 1;
    } flags;

    std::string toString()
    {
        std::stringstream ss;
        ss << "I:"
            << std::hex
            << std::setfill('0')
            << std::setw(sizeof(uint32_t) * 2)
            << input;

        ss << "  O:"
            << std::hex
            << std::setfill('0')
            << std::setw(sizeof(uint32_t) * 2)
            << output;

        ss << " flags:"
            << flags.error;
        return ss.str();
    }
};