#pragma once

#include "Server.h"
#include "App_controler.hpp"
#include "App_builder.hpp"

#include <boost/json.hpp>


#include <filesystem>
#include <chrono>
#include <string>
#include <memory>




struct PLC_TCP_server_config{
	std::filesystem::path user_app_root;
	App_controler* app_controler;
};



class PLC_server_connection_handle : public Server_connection_handle {

	const PLC_TCP_server_config config;

public:
	// IMPORTANT - without this constructor code wont compile
	// and compiler will show strange error 
	// NOTE - constructor must be public
	PLC_server_connection_handle(boost::asio::ip::tcp::socket& client, PLC_TCP_server_config _config) : 
		Server_connection_handle(client),
		config(_config) {}



private:


	boost::json::stream_parser jsonParser;


private:
	void parseJsonStream(char* data, size_t len) {
		boost::json::value json;

		std::error_code err;
		size_t start = 0;
		for (size_t i = 0; i < len; i++) {
			if (data[i] == '\n') {
				jsonParser.write(&data[start], i - start, err);
				jsonParser.finish(err);

				if (jsonParser.done()) {
					auto json = jsonParser.release();
					//DataFrame data = boost::json::value_to<DataFrame>(json);
					onReadCommand(json);
				}
				else {
					onReadCommand(err);
				}
				jsonParser.reset();
				start = i;
			}
		}
		jsonParser.write(&data[start], len - start, err);
	}


	void onStart() {}


	void onRead(const boost::system::error_code& error, size_t bytes_received, uint8_t* data) {
		parseJsonStream((char*)data, bytes_received);
	}


	void sendJson(const boost::json::value& js){
		std::string str = serialize(boost::json::value_from(js)) + "\n";
		write((const uint8_t*)str.c_str(), str.size());
	}


	void SendErrorMsg(std::string msg){
		boost::json::value js = {{"Error",msg}};
		sendJson(js);
	}


	void onReadCommand(const std::error_code& err) {
		std::cout << "ERR - " << err.message() << "\n";
	}


	void onReadCommand(const boost::json::value& data_frame) {
		std::string cmd;

		if(!data_frame.is_object()) {
			SendErrorMsg("Parsing Error: Json data frame must be an object"); 
			return; 
		}

		const boost::json::object& data_frame_obj = data_frame.as_object();

		auto js_cmd = data_frame_obj.if_contains("Cmd");
		if(!js_cmd){
			SendErrorMsg("Parsing Error: Missing \"Cmd\" field"); 
			return; 
		}
		if(!js_cmd->is_string()){
			SendErrorMsg("Parsing Error: \"Cmd\" field must be string"); 
			return; 
		}
		
		cmd = js_cmd->as_string();

		if      (cmd == "UNNOWN")       commandUnnown(cmd);
		else if (cmd == "PING")         commandPing(cmd);
		else if (cmd == "APP_START")    commandAppStart(data_frame_obj, cmd);
		else if (cmd == "APP_STOP")     commandAppStop(data_frame_obj, cmd);
		else if (cmd == "APP_RESUME")   commandNotImplemented(data_frame_obj, cmd);
		else if (cmd == "APP_PAUSE")    commandNotImplemented(data_frame_obj, cmd);
		else if (cmd == "APP_STATUS")   commandAppStatus(data_frame_obj, cmd);
		else if (cmd == "APP_BUILD")    commandAppBuild(data_frame_obj, cmd);
		else if (cmd == "FILE_LIST")    commandFileList(cmd);
		else if (cmd == "FILE_WRITE")   commandFileWrite(data_frame_obj, cmd);
		else if (cmd == "FILE_READ")    commandFileRead(data_frame_obj, cmd);
		else if (cmd == "MODULE_LIST")  commandNotImplemented(data_frame_obj, cmd);
		else if (cmd == "TAG_LIST")     commandNotImplemented(data_frame_obj, cmd);
		else if (cmd == "TAG_GET")      commandNotImplemented(data_frame_obj, cmd);
		else if (cmd == "TAG_SET")      commandNotImplemented(data_frame_obj, cmd);
		else {
			sendJson(jsonResponseERR(cmd, "Unnown Command"));
		}

	}

	


	boost::json::value jsonResponseOK(std::string cmd, std::string key, auto value){
		boost::json::object response =
			{	
				{"Cmd",cmd},
				{"Result","OK"},
				{key, value}
			};
		return response;
	}

	boost::json::value jsonResponseOK(std::string cmd){
		boost::json::value response =
			{	
				{"Cmd",cmd}, 
				{"Result","OK"}
			};
		return response;
	}

	boost::json::value jsonResponseERR(std::string cmd, std::string msg){
		boost::json::value response =
			{	
				{"Cmd",cmd}, 
				{"Result","ERROR"}, 
				{"Msg", msg}
			};
		return response;
	}


	void commandAppStart(const boost::json::object& data_frame, const std::string& cmd){
		if(config.app_controler == nullptr){
			sendJson(jsonResponseERR(cmd, "Cannot Start App"));
			return;
		}
		config.app_controler->AppStart();
		sendJson(jsonResponseOK(cmd));
	}

	void commandAppStop(const boost::json::object& data_frame, const std::string& cmd){
		if(config.app_controler == nullptr){
			sendJson(jsonResponseERR(cmd, "Cannot Stop App"));
			return;
		}
		config.app_controler->AppStop();
		sendJson(jsonResponseOK(cmd));
	}

	void commandAppStatus(const boost::json::object& data_frame, const std::string& cmd){
		if(config.app_controler == nullptr){
			sendJson(jsonResponseERR(cmd, "Cannot Check App Status"));
			return;
		}
		bool is_running = config.app_controler->AppIsRunning();
		std::string status = is_running ? "RUNNING" : "STOPPED";
		
		sendJson(jsonResponseOK(cmd, "Status", status));
	}

	// TODO:
	// This function is synchronous GARBAGE.
	// It can easily freeze whole server for significant time.
	// 
	//
	// FIX builder.build(); function in future
	// 

	void commandAppBuild(const boost::json::object& data_frame, const std::string& cmd){
		AppBuilder builder;
		builder.loadBuildConf(config.user_app_root / "build.conf");
		std::vector<AppBuilder::BuildResult> result = builder.build();
		
		sendJson(jsonResponseOK(cmd, "CompilationResult", result));
	}


	void commandNotImplemented(const boost::json::object& data_frame, const std::string& cmd){
		sendJson(jsonResponseERR(cmd, "Command not yet implemented"));
	}


	void commandUnnown(const std::string& cmd){
		sendJson(jsonResponseOK(cmd, "Msg", "XD"));
	}


	void commandPing(const std::string& cmd){
		sendJson(jsonResponseOK(cmd, "Msg", "Pong"));
	}


	bool readFileHex(const std::string& path, std::string* hex_file);
	bool writeFileHex(const std::string& path,const std::string& hex_str);

	void commandFileList(const std::string& cmd);
	void commandFileRead(boost::json::object data_frame, const std::string& cmd);
	void commandFileWrite(boost::json::object data_frame, const std::string& cmd);



	void onWrite(const boost::system::error_code& error, std::size_t bytes_transferred) {

	}


};



// "Attach" connection handle to server class
typedef TCP_server<PLC_server_connection_handle, PLC_TCP_server_config> PLC_TCP_server;

