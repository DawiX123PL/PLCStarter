#pragma once

#include "Server.h"
#include "App_controler.hpp"

#include <boost/json.hpp>


#include <filesystem>
#include <chrono>
#include <codecvt>
#include <string>
#include <memory>



class PLC_server_connection_handle : public Server_connection_handle {
public:
	// IMPORTANT - without this constructor code wont compile
	// and compiler will show strange error 
	// NOTE - constructor must be public
	PLC_server_connection_handle(boost::asio::ip::tcp::socket& client) : Server_connection_handle(client) {}

	static App_controler* app_controler;

private:


	boost::json::stream_parser jsonParser;


private:
	void parseJsonStream(char* data, int len) {
		boost::json::value json;

		std::error_code err;
		int start = 0;
		for (int i = 0; i < len; i++) {
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
		else if (cmd == "APP_START")    commandNotImplemented(data_frame_obj, cmd);
		else if (cmd == "APP_STOP")     commandNotImplemented(data_frame_obj, cmd);
		else if (cmd == "APP_RESUME")   commandNotImplemented(data_frame_obj, cmd);
		else if (cmd == "APP_PAUSE")    commandNotImplemented(data_frame_obj, cmd);
		else if (cmd == "APP_BUILD")    commandNotImplemented(data_frame_obj, cmd);
		else if (cmd == "FILE_LIST")    commandFileList(cmd);
		else if (cmd == "FILE_WRITE")   commandFileWrite(data_frame_obj, cmd);
		else if (cmd == "FILE_READ")    commandFileRead(data_frame_obj, cmd);
		else if (cmd == "MODULE_LIST")  commandNotImplemented(data_frame_obj, cmd);
		else if (cmd == "TAG_LIST")     commandNotImplemented(data_frame_obj, cmd);
		else if (cmd == "TAG_GET")      commandNotImplemented(data_frame_obj, cmd);
		else if (cmd == "TAG_SET")      commandNotImplemented(data_frame_obj, cmd);
		else {
			boost::json::value response = {{"Cmd",cmd}, {"Result","ERROR"}, {"Msg", "Unnown Command"}};
			sendJson(response);
		}

	}


	void commandNotImplemented(const boost::json::object& data_frame, const std::string& cmd){
		boost::json::value response = {{"Cmd",cmd}, {"Result","ERROR"}, {"Msg", "Command not yet implemented"}};
		sendJson(response);
	}


	void commandUnnown(const std::string& cmd){
		boost::json::value response = {{"Cmd",cmd}, {"Result","OK"}, {"Msg", "XD"}};
		sendJson(response);
	}


	void commandPing(const std::string& cmd){
		boost::json::value response = {{"Cmd","cmd"}, {"Result","OK"}, {"Msg", "PONG"}};
		sendJson(response);
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
typedef TCP_server<PLC_server_connection_handle> PLC_TCP_server;

