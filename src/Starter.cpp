#include <iostream>
#include "Server.h"
#include "PLCServer.h"
#include "boost/json/src.hpp"
#include "StarterConfig.h"



int main() {


	Starter_config config = read_config("./starter.config");
	

	int port = config.server.port;

	PLC_TCP_server server(port);
	server.Start();

	std::cout << "Starting server at port: " << port << "\n";

	getchar(); // this is just to stop program for debugging

	std::cout << "Shutting down server \n";
	server.Stop();
	server.Join();
	std::cout << "Server stopped workign\n";
}