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
		else if (cmd == "FILE_LIST")    commandFileList(cmd);  // commandNotImplemented(data_frame, cmd);
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


	void commandFileList(const std::string& cmd){
		std::string path = "./userAppRoot/root";

		std::filesystem::recursive_directory_iterator dir_iter(path);

		boost::json::array result_array;

		for(auto dir_entry : dir_iter){

			std::error_code err;

			// IMPORTANT 
			// time_since_epoch() does not guarantee to return time duration since unix epoch util c++20.
			//
			// TODO: change it later or change c++ version to c++20
			//
			namespace ch = std::chrono;
			auto modification_time_point = ch::clock_cast<ch::system_clock>(dir_entry.last_write_time());
			auto modification_time = ch::duration_cast<ch::seconds>(modification_time_point.time_since_epoch()).count();


			std::string type;
			if(dir_entry.is_directory()) 
				type = "Dir";
			else if(dir_entry.is_regular_file()) 
				type = "File";
			else 
				type = "Other";

			std::wstring_convert<std::codecvt<char32_t,char,std::mbstate_t>,char32_t> convert32;
			
			auto relative_path = std::filesystem::relative(dir_entry.path(), path);
			auto path = convert32.to_bytes(relative_path.u32string());

			boost::json::object dir_entry_js = 
				{
					{"Name", path }, 
					{"Date", modification_time},
					{"Type", type }
				};
			
			result_array.push_back(dir_entry_js);
		}

		boost::json::value response = { {"Cmd",cmd}, {"Result","OK"}, {"DirEntry", result_array} };
		sendJson(response);
	}


	bool readFileHex(const std::string& path, std::string* hex_file) {

		try {

			std::fstream file(path, std::ios::binary | std::ios::in);

			if (!file.is_open()) {
				return false;
			}

			file.seekg(0,std::ios::end);
			int count = file.tellg();
			file.seekg(0,std::ios::beg);

			std::unique_ptr buf_unique = std::make_unique<char[]>(count); // not using 'new' to prevent memory leaks
			char* buf = buf_unique.get();

			file.read(buf, count);

			std::unique_ptr hex_buf_unique = std::make_unique<char[]>(count*2); // not using 'new' to prevent memory leaks
			char* hex_buf = hex_buf_unique.get();


			for (int i = 0; i < count; i++) {
			
				char lower = buf[i] & 0x0f;
				char higher = buf[i] >> 4;

				hex_buf[i * 2] = higher < 10 ? '0' + higher : 'a' - 0xa + higher;
				hex_buf[i * 2 + 1] = lower < 10 ? '0' + lower : 'a' - 0xa + lower;
			}

			*hex_file = std::string(hex_buf, count * 2);

		}
		catch (...) {
			return false;
		}


		return true;
	}


	void commandFileRead(boost::json::object data_frame, const std::string& cmd) {

		std::filesystem::path root = "./userAppRoot/root";
		
		std::string name;

		if (data_frame.contains("FileName"))
			if (auto file_name = data_frame.at("FileName").if_string())
				name = *file_name;



		if (name == "") {
			boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","FileName is not specified"} };
			sendJson(response);
			return;
		}

		std::filesystem::path path = root.append(name);

		if (!std::filesystem::exists(path)) {
			boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","File \"" + name + "\" does not exists"} };
			sendJson(response);
			return;
		}

		std::string file_content;
		if (!readFileHex(path.string(), &file_content)) {
			boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","Cannot read file"} };
			sendJson(response);
			return;
		}

		boost::json::value response = { {"Cmd",cmd}, {"Result","Ok"}, {"Data",file_content} };
		sendJson(response);


	}


	bool writeFileHex(const std::string& path,const std::string& hex_str){

		if ( hex_str.size() % 2 != 0){
			return false;
		}

		const char* hex = hex_str.c_str();
		int count = hex_str.size() / 2;


		std::unique_ptr data_unique_ptr = std::make_unique<char[]>(count);
		char* data = data_unique_ptr.get();

		for(int i = 0; i < count; i++){
			
			char lower = hex[i * 2 + 1];
			char higher = hex[i * 2];;

			char byte = 0;
			if(higher >= '0' && higher <= '9') byte = higher - '0';
			else if(higher >= 'a' && higher <= 'f') byte = higher - 'a' + 0xa;
			else if(higher >= 'A' && higher <= 'F') byte = higher - 'A' + 0xa;
			else higher += 0;

			byte <<= 4;
			if(lower >= '0' && lower <= '9') byte += lower - '0';
			else if(lower >= 'a' && lower <= 'f') byte += lower - 'a' + 0xa;
			else if(lower >= 'A' && lower <= 'F') byte += lower - 'A' + 0xa;
			else byte += 0;

			data[i] = byte;
		}

		// send data to file;

		try{


			std::fstream file(path, std::ios::out | std::ios::binary);

			if(!file.is_open()) return false;
			if(file.bad()) return false;
						
			file.write(data, count);


		}catch(...){
			return false;
		}

		return true;
	}


	void commandFileWrite(boost::json::object data_frame, const std::string& cmd){
		std::filesystem::path root = "./userAppRoot/root";
		
		std::string name;

		if (data_frame.contains("FileName")){
			if (auto file_name = data_frame.at("FileName").if_string()){
				name = *file_name;
			}else{
				boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","Field \"FileName\" must be string"} };
				sendJson(response);
				return;
			}
		}else{
			boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","Missing field \"FileName\""} };
			sendJson(response);
			return;
		}

		if(name == ""){
			boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","\"FileName\" Cannot be empty string"} };
			sendJson(response);
			return;
		}
		
		std::string data_hex;

		if (data_frame.contains("Data")){
			if (auto data_hex_str = data_frame.at("Data").if_string()){
				data_hex = *data_hex_str;
			}else{
				boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","Field \"Data\" must be string"} };
				sendJson(response);
				return;
			}
		}else{
			boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","Missing field \"Data\""} };
			sendJson(response);
			return;
		}

		std::filesystem::path path = root;
		path.append(name);
		if (!writeFileHex(path.string(), data_hex)) {
			boost::json::value response = { {"Cmd",cmd}, {"Result","ERROR"}, {"Msg","Cound not write file"} };
			sendJson(response);
			return;
		}

		boost::json::value response = { {"Cmd",cmd}, {"Result","OK"} };
		sendJson(response);
		return;

	}
	



	void onWrite(const boost::system::error_code& error, std::size_t bytes_transferred) {

	}


};


// "Attach" connection handle to server class
typedef TCP_server<PLC_server_connection_handle> PLC_TCP_server;

