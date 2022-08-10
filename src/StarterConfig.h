#pragma once
#include <inttypes.h>
#include <string>
#include <boost/json.hpp>
#include <fstream>
#include <iostream>


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
		std::string savePath = "."; // path to save and launch app
	}app;

};


// name is self explanatory
Starter_config read_config(std::string path);


