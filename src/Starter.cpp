#include <iostream>
#include <chrono>

#include "Server.h"
#include "PLCServer.h"
#include "StarterConfig.h"
#include "PLC_IO_bridge.hpp"
#include "App_controler.hpp"



App_controler* PLC_server_connection_handle::app_controler = nullptr;



int main() {

	Starter_config config = read_config("./starter.config");
	

	App_controler app;
	//app.setPath(config.app.projRoot);
	//app.setExec(config.app.executable);

	PLC_server_connection_handle::app_controler = &app;
	PLC_TCP_server server(config.server.port);

	app.Start();
	server.Start();

	std::cout << "Starting server at port: " << config.server.port << "\n";

	getchar(); // this is just to stop program for debugging

	app.Stop();
	server.Stop();
	
	std::cout << "Shutting down server \n";
	app.Join();
	server.Join();
	std::cout << "Server stopped workign\n";
}