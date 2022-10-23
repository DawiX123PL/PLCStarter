#pragma once

#include <string>
#include <mutex>
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <memory>

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

public:
	App_controler(): app_status(AppStatus::STOPPED){}



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


	void threadJob() {

		std::unique_ptr<boost::process::child> app;

		PLC_IO_bridge io_bridge;	
		int i = 0;


		while(IsRun()){

			
			if(app_status_mutex.try_lock()){
				// locked
				
				bool start_app = app_status == AppStatus::START;
				bool stop_app = app_status == AppStatus::STOP;
				if(start_app) app_status = AppStatus::RUNNING;
				if(stop_app) app_status = AppStatus::STOPPED;
				app_status_mutex.unlock();

				if( start_app ){
					
					app = std::make_unique<boost::process::child>(
						"./userAppRoot/root/build/app.exe", 
						boost::process::std_out > boost::process::null, 
						boost::process::std_err > boost::process::null, 
						boost::process::std_in < boost::process::null
						);
				}

				if( stop_app ){
					std::error_code err;
					if(app) app->terminate(err);
					
				}

			}

			if(app && !app->running()){
				if(app_status_mutex.try_lock()){
					app_status = AppStatus::STOPPED;
					app.reset();
					app_status_mutex.unlock();
				}
			}

			if(app && app->running()){
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

	}


};
