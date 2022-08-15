#pragma once

#include <string>
#include <mutex>
#include <boost/process/child.hpp>
#include <iostream>

#include "thread.h"
#include "PLC_IO_bridge.hpp"


class App_controler : public Thread {
private:


	std::mutex mutex;
	boost::process::child userApp;
	std::string userAppPath;

public:
	void setPath(std::string path) {
		mutex.lock();
		userAppPath = path;
		mutex.unlock();
	}

	void startApp() {
		mutex.lock();
		userApp = boost::process::child(userAppPath);
		mutex.unlock();
	}

	void stopApp() {
		mutex.lock();
		std::error_code err;
		userApp.terminate(err);
		mutex.unlock();
	}

private:
	void threadJob() {
		PLC_IO_bridge io_bridge(boost::interprocess::open_or_create);

		std::vector<PLC_IO_Module_Data> newBlocks;
		newBlocks.push_back({});
		io_bridge.set_IO_Blocks(newBlocks);



		while (IsRun()) {
			auto blocks = io_bridge.get_IO_Blocks();
			for (auto b : blocks) {
				std::cout << b.toString() << "\n";
			}
			std::cout << "\n";

			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}

	}


};
