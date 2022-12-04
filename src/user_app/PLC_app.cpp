#include "../PLC_IO_bridge.hpp"
#include "PLC_app.hpp"


PLC_IO_bridge plc_io_bridge1;

bool PLC::LoopStart(){
    PLC_IO_bridge::wait_result result = plc_io_bridge1.wait_until_loop_started();
    if(result == PLC_IO_bridge::wait_result::OK) return true;
    else return false;
}

bool PLC::LoopEnd() {
    plc_io_bridge1.end_loop();
    return true;
}

PLC::IOmoduleData PLC::GetIO(size_t module_nr){
    if(module_nr >= plc_io_bridge1.getIOModulesSize()){
        return IOmoduleData{};
    }else{
        IOmoduleData data;
        data.input = plc_io_bridge1.getIOModulesAdress()->input;
        data.output = plc_io_bridge1.getIOModulesAdress()->output;
        return data;
    }
}


void PLC::SetIO(IOmoduleData data, size_t module_nr){
    if(module_nr < plc_io_bridge1.getIOModulesSize()){
        plc_io_bridge1.getIOModulesAdress()->input = data.input;
        plc_io_bridge1.getIOModulesAdress()->output = data.output;
    }
}





