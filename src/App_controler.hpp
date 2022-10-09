#pragma once

#include <string>
#include <mutex>
#include <boost/process/child.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>

#include "thread.h"
#include "PLC_IO_bridge.hpp"


class App_controler : public Thread {

private:
	void threadJob() {
		PLC_IO_bridge io_bridge;

		int i = 0;

		while (IsRun()) {
			auto delay = std::chrono::high_resolution_clock::now() + std::chrono::seconds(1);
			io_bridge.start_loop(delay);
			PLC_IO_bridge::wait_result err = io_bridge.wait_until_loop_finished_or(delay);

			if (err == PLC_IO_bridge::wait_result::OK) {
				std::cout << "OK  - " << i++ << "\n";
			}
			else {
				std::cout << "ERR - " << i++ << "\n";
			}

		}

	}


};
