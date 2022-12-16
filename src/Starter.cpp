#include <iostream>
#include <chrono>
#include <filesystem>

#include "Server.h"
#include "PLCServer.h"
#include "StarterConfig.h"
#include "PLC_IO_bridge.hpp"
#include "App_controler.hpp"




int main(int argc, char* argv[]) {


	// get application path
	std::filesystem::path exe_path = argv[0];
	std::filesystem::path exe_dir = exe_path.parent_path();


	// read config
	std::filesystem::path config_path = exe_dir / "starter.config";
	Starter_config config = read_config(config_path);

	// setup modules
	std::filesystem::path include_dir = config.compilation.include_directory;
	if(include_dir.is_absolute()){
		AppBuilder::include_dir = include_dir;
	}else{
		AppBuilder::include_dir = exe_dir / include_dir;
	}

	std::filesystem::path library_path = config.compilation.library_path;
	if(library_path.is_absolute()){
		AppBuilder::library_path = library_path;
	}else{
		AppBuilder::library_path = exe_dir / library_path;
	}

	App_controler app;

	PLC_TCP_server server(config.server.port);

	PLC_TCP_server_config server_config{};

	std::filesystem::path app_root_path = config.app.projRoot;
	if (app_root_path.is_absolute()){
		server_config.user_app_root = app_root_path;
		app.SetAppPath(app_root_path / "build" / "app.exe");
	}else{
		server_config.user_app_root = exe_dir / app_root_path;
		app.SetAppPath(exe_dir / app_root_path / "build" / "app.exe");
	}

	server_config.app_controler = &app;
	server.set_config(server_config);

	// start modules
	app.Start();
	server.Start();

	std::cout << "Starting server at port: " << config.server.port << "\n";

	getchar(); // this is just to stop program for debugging

	app.Stop();
	server.Stop();
	
	// shutdown
	std::cout << "Shutting down server \n";
	app.Join();
	server.Join();
	std::cout << "Server stopped workign\n";
}