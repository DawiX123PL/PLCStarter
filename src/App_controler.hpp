#pragma once

#include <string>
#include <mutex>
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>
#include <bitset>

#include "thread.h"
#include "PLC_IO_bridge.hpp"




class App_controler : public Thread {


	// https://asciiflow.com/ - very cool application

//               ┌──────┐
//     ┌─────────┤START ├────────┐
//     │         └────┬─┘        ▼
// ┌───┴───┐       ▲  │      ┌───────┐
// │STOPPED│       │  │      │RUNNING│
// └───────┘       │  ▼      └───┬───┘
//     ▲         ┌─┴────┐        │
//     └─────────┤ STOP │◄───────┘
//               └──────┘
	enum class AppStatus {
		STOPPED,
		RUNNING,
		START, // STOPPED -> START -> RUNNING
		STOP, // RUNNING -> STOP -> STOPPED
	};


	std::mutex app_status_mutex;
	AppStatus app_status;
	std::filesystem::path app_path;

public:


	App_controler(): app_status(AppStatus::STOPPED){}


	void SetAppPath(std::filesystem::path path) {
		app_status_mutex.lock();
		app_path = path;
		app_status_mutex.unlock();
	}


	void AppStart(){
		app_status_mutex.lock();
		if(app_status != AppStatus::RUNNING){
			app_status = AppStatus::START;
		}
		app_status_mutex.unlock();
	}

	void AppStop(){
		app_status_mutex.lock();
		if(app_status != AppStatus::STOPPED){
			app_status = AppStatus::STOP;
		}
		app_status_mutex.unlock();
	}

	bool AppIsRunning(){
		app_status_mutex.lock();
		bool is_running = false;
		if(app_status == AppStatus::RUNNING || app_status == AppStatus::START){
			is_running = true;
		}
		app_status_mutex.unlock();
		return is_running;
	}


private:

	void threadJob();

};
