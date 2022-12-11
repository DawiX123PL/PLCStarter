#pragma once
#include <inttypes.h>
#include <string>
#include <boost/json.hpp>
#include <fstream>
#include <iostream>
#include <filesystem>


//	REMEMBER !!!!
// 
//  for more parameters update tag_invoke() function
//
//




// struct hold parameters from config file
struct Starter_config {

	struct Server {
		uint16_t port = 8000; // server port
	} server;

	struct App {
		bool autoStart = false;
		std::string projRoot = "./userApp"; // path to save all files
		std::string executable = "app.exe"; // name of executable to executable
	}app;

	struct Compilation{
		std::string library_path = "---";
		std::string include_directory = "---";
	}compilation;

};


// name is self explanatory
Starter_config read_config(std::filesystem::path path);


