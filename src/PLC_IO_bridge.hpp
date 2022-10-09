#pragma once

#include <mutex>
#include <chrono>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "PLC_IO_data.hpp"



// #define PLC_IO_BRIDGE_CREATE    1
// #define PLC_IO_BRIDGE_CONNECT   1


#if PLC_IO_BRIDGE_CREATE == 0 && PLC_IO_BRIDGE_CONNECT == 0
#error You have to define: PLC_IO_BRIDGE_CREATE=1 or PLC_IO_BRIDGE_CONNECT=1 (but not both).
#endif


#if PLC_IO_BRIDGE_CREATE == 1 && PLC_IO_BRIDGE_CONNECT == 1
#error You have to define only one of these: PLC_IO_BRIDGE_CREATE=1 or PLC_IO_BRIDGE_CONNECT=1.
#endif


class PLC_IO_bridge
{

    boost::interprocess::shared_memory_object* shared_mem;
    boost::interprocess::mapped_region* mapped_region;

    constexpr static char shared_mem_name[] = "PLC_IO_bridge_shared_memory";

    struct SharedData{
        boost::interprocess::interprocess_mutex mutex;
        
        enum class UserAppStatus{
            LOOP_START,
            LOOP_EXECUTING,
            LOOP_FINISHED
        }user_app_status;

        PLC_io_module io_module[1];
        PLC_io_tag io_tag[1];
    };

    constexpr static size_t shared_mem_size = sizeof(SharedData);

    SharedData* shared_data;

public:

    #if PLC_IO_BRIDGE_CREATE == 1

    PLC_IO_bridge(){
        namespace ip = boost::interprocess;
        // create shared memory
        shared_mem = new ip::shared_memory_object(ip::open_or_create, shared_mem_name, ip::read_write);

        // allocate memory with specified size
        shared_mem->truncate(shared_mem_size);

        // map shared memory region
        mapped_region = new ip::mapped_region(*shared_mem, ip::read_write);

        // get memory adress and fill memory with correct data
        void* adress = mapped_region->get_address();
        shared_data = new(adress) SharedData;
    }

    ~PLC_IO_bridge(){
        namespace ip = boost::interprocess;
        // remove unused resources
        delete shared_mem; 
        delete mapped_region;

        // remove shared memory
        ip::shared_memory_object::remove(shared_mem_name);
    }

    #endif



    #if PLC_IO_BRIDGE_CONNECT == 1

    PLC_IO_bridge(){
        namespace ip = boost::interprocess;
        // create shared memory
        shared_mem = new ip::shared_memory_object(ip::open_only, shared_mem_name, ip::read_write);

        // map shared memory region
        mapped_region = new ip::mapped_region(*shared_mem, ip::read_write);

        // get memory adress
        shared_data = (SharedData*)mapped_region->get_address();
    }

    ~PLC_IO_bridge(){
        namespace ip = boost::interprocess;
        // remove unused resources
        delete shared_mem; 
        delete mapped_region;
    }

    #endif

private:


public:

    enum class wait_result{
        OK,
        TIMEOUT,
        WAIT_ERROR
    };

    #if PLC_IO_BRIDGE_CREATE == 1

    template< class Clock, class Duration >
    wait_result wait_until_loop_finished_or( const std::chrono::time_point<Clock,Duration>& time ){


        while(std::chrono::high_resolution_clock::now() < time){

            if( shared_data->mutex.try_lock_until(time) ){
                // mutex has been acquired

                if (shared_data->user_app_status == SharedData::UserAppStatus::LOOP_FINISHED){
                    shared_data->mutex.unlock();
                    return wait_result::OK;
                }else{
                    shared_data->mutex.unlock();
                    std::this_thread::yield();
                    continue;
                }

            }else{
                // timeout 
                return wait_result::TIMEOUT;
            }

        }
        // timeout
        return wait_result::TIMEOUT;
    }

    template< class Clock, class Duration >
    wait_result start_loop(const std::chrono::time_point<Clock,Duration>& time){
        if( shared_data->mutex.try_lock_until(time) ){
            // mutex has been acquired

            shared_data->user_app_status = SharedData::UserAppStatus::LOOP_START;
            shared_data->mutex.unlock();
            return wait_result::OK;

        }else{
            // timeout 
            return wait_result::TIMEOUT;
        }
    }

    #endif


    #if PLC_IO_BRIDGE_CONNECT == 1

    wait_result wait_until_loop_started(){

        end_loop();

        while(true){
            shared_data->mutex.lock();
                
            if(shared_data->user_app_status == SharedData::UserAppStatus::LOOP_START){
                shared_data->user_app_status = SharedData::UserAppStatus::LOOP_EXECUTING;
                shared_data->mutex.unlock();
                return wait_result::OK;
            }

            shared_data->mutex.unlock();
            std::this_thread::yield();
        }

    }

    void end_loop() {
        shared_data->mutex.lock();
        shared_data->user_app_status = SharedData::UserAppStatus::LOOP_FINISHED;
        shared_data->mutex.unlock();
    }

    #endif

};
