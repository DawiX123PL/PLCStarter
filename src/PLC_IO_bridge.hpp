#pragma once
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>

#include "PLC_IO_module_data.hpp"


class PLC_IO_bridge
{

public:
    

    const bool isCreate;

    constexpr static char PLC_IO_PORT_name[] = "PLC_IO_Port_name";
    constexpr static char PLC_IO_PORT_mutex[] = "PLC_IO_Port_mutex";
    constexpr static size_t PLC_IO_PORT_size = 1024 * 1024; // 1MB

    // this is only to not deal with long type names
    typedef boost::interprocess::allocator<PLC_IO_Module_Data, boost::interprocess::managed_shared_memory::segment_manager> IO_block_allocator_t;
    typedef boost::interprocess::vector<PLC_IO_Module_Data, IO_block_allocator_t> IO_block_vector_t;

    // MUTEX - VERY IMPORTANT
    boost::interprocess::named_mutex *mutex;

    // important variables
    boost::interprocess::managed_shared_memory sharedMem;
    IO_block_allocator_t *IO_block_allocator;
    IO_block_vector_t *IO_blocks;

public:
    PLC_IO_bridge(boost::interprocess::open_or_create_t) : isCreate(true) { init(); }
    PLC_IO_bridge(boost::interprocess::open_only_t) : isCreate(false) { init(); }

    ~PLC_IO_bridge()
    {
        // VERY IMPORTANT 
        // remove mutex if created
        if (isCreate)
            boost::interprocess::named_mutex::remove(PLC_IO_PORT_mutex);

        // remove shared mem if created
        if (isCreate)
            boost::interprocess::shared_memory_object::remove(PLC_IO_PORT_name);

        // delete allocator
        delete IO_block_allocator;

        delete mutex;
    }

    void set_IO_block(PLC_IO_Module_Data block, size_t index) {
        namespace ip = boost::interprocess;

        ip::scoped_lock<ip::named_mutex> mutLock(*mutex);
        if (mutLock) {
            IO_blocks->at(index) = block;
        }
    }

    PLC_IO_Module_Data get_IO_block(size_t index) {
        namespace ip = boost::interprocess;

        ip::scoped_lock<ip::named_mutex> mutLock(*mutex);
        if (mutLock) {
            return IO_blocks->at(index);
        }
    }


    void set_IO_Blocks(std::vector<PLC_IO_Module_Data> blocks) {
        namespace ip = boost::interprocess;

        ip::scoped_lock<ip::named_mutex> mutLock(*mutex);
        if (mutLock) {
            IO_blocks->reserve(blocks.size());

            for (int i = 0; i < blocks.size(); i++) {
                IO_blocks->push_back(blocks[i]);
            }

        }
    }


    std::vector<PLC_IO_Module_Data> get_IO_Blocks() {
        namespace ip = boost::interprocess;

        std::vector<PLC_IO_Module_Data> blocks;

        ip::scoped_lock<ip::named_mutex> mutLock(*mutex);
        if (mutLock) {

            blocks.reserve(IO_blocks->size());
            for (int i = 0; i < IO_blocks->size(); i++) {
                blocks.push_back(IO_blocks->at(i));
            }
                
        }
        return blocks;
    }


private:
    void init()
    {
        namespace ip = boost::interprocess;

        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // MOST IMPORTANT - CREATE MUTEX FIRST
        if (isCreate)
        {
            ip::named_mutex::remove(PLC_IO_PORT_mutex);
            mutex = new ip::named_mutex(ip::open_or_create, PLC_IO_PORT_mutex);
        }
        else
        {
            mutex = new ip::named_mutex(ip::open_only, PLC_IO_PORT_mutex);
        }

        // create shared memory object;
        if (isCreate) {
            ip::shared_memory_object::remove(PLC_IO_PORT_name);
            sharedMem = ip::managed_shared_memory(ip::open_or_create, PLC_IO_PORT_name, PLC_IO_PORT_size);
        }
        else {
            sharedMem = ip::managed_shared_memory(ip::open_only, PLC_IO_PORT_name);
        }


        // create allocator for IO_blocks
        IO_block_allocator = new IO_block_allocator_t(sharedMem.get_segment_manager());

        // ... and vector for IO_blocks
        if (isCreate) {
            IO_blocks = sharedMem.construct<IO_block_vector_t>("IO_block_vector")(*IO_block_allocator);
        }
        else {
            IO_blocks = sharedMem.find<IO_block_vector_t>("IO_block_vector").first;            
        }
        
    }
};
