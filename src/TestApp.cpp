#include <iostream>
#include <thread>
#include "PLC_IO_bridge.hpp"



int main() {

	PLC_IO_bridge io_bridge(boost::interprocess::open_only);

	PLC_IO_Module_Data block{};


	while (true) {
		block.input = 0xff00ab;
		io_bridge.set_IO_block(block, 0);

		std::cout << "set\n";
		std::this_thread::sleep_for(std::chrono::milliseconds(4000));

		block.input = 0x0;
		io_bridge.set_IO_block(block, 0);
		std::cout << "reset\n";
		std::this_thread::sleep_for(std::chrono::milliseconds(4000));
	}
}