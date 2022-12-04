#pragma once


namespace PLC{

    struct IOmoduleData{
        uint32_t input;
        uint32_t output;
    };

    bool LoopStart();
    bool LoopEnd();

    IOmoduleData GetIO(size_t module_nr = 0);
    void SetIO(IOmoduleData data, size_t module_nr = 0);

}










