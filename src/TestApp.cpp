#include <iostream>
#include <thread>
#include "PLC_IO_bridge.hpp"



int main() {

	PLC_IO_bridge io_bridge;

	int i = 0;

	while (true) {
		io_bridge.wait_until_loop_started();

		std::cout << "LOOP nr. " << i++ << "\n";

		io_bridge.end_loop();
	}
}