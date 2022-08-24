#pragma once

#include "Server.h"
#include "App_controler.hpp"

#include <boost/json.hpp>


class PLC_server_connection_handle : public Server_connection_handle {
public:
	// IMPORTANT - without this constructor code wont compile
	// and compiler will show strange error 
	// NOTE - constructor must be public
	PLC_server_connection_handle(boost::asio::ip::tcp::socket& client) : Server_connection_handle(client) {}

	static App_controler* app_controler;

private:


	boost::json::stream_parser jsonParser;

public:
	struct DataFrame {
		enum class Command{
			UNNOWN,         // unnown command - always send error in response
			SERVER_PING,    // ping device
			SERVER_PONG,    // response to ping
			APP_START,      // start or resume app
			APP_STOP,       // kill app
			APP_RESUME,     // resume app - error it app is stopped
			APP_PAUSE,      // pause app - error if app is soppped
			APP_BUILD,      // build user application
			MODULE_LIST,    // list of all modules connected to device
			TAG_LIST,       // get list of tags
			TAG_GET,        // get tag value
			TAG_SET,        // set tag to value 
			FILE_LIST,      // list of all files in user root dir
			FILE_SAVE,		// save file
			FILE_READ,      // read file
		} cmd = Command::UNNOWN;
		
		enum class Response {
			NONE,
			OK,
			ERR
		}response = Response::NONE;

		std::string msg;
		std::vector<std::string> fileList;
		std::string path;
		std::string data;

		DataFrame(){}
		DataFrame(Command c) :cmd(c), msg("") {}
		DataFrame(Command c, Response r):cmd(c), response(r), msg("") {}
		DataFrame(Command c, std::string m) :cmd(c), msg(m) {}
		DataFrame(Command c, Response r, std::string m) :cmd(c), response(r), msg(m) {}
	};


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
					DataFrame data = boost::json::value_to<DataFrame>(json);
					onReadCommand(data);
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


	void onReadCommand(const std::error_code& err) {
		std::cout << "ERR - " << err.message() << "\n";
	}

	void sendCommand(DataFrame data) {
		auto str = serialize(boost::json::value_from(data)) + "\n";
		write((const uint8_t*)str.c_str(), str.size());
	}



	void onReadCommand(DataFrame data) {

		typedef DataFrame::Command DF_cmd;
		typedef DataFrame::Response DF_r;


		switch (data.cmd)
		{
		case(DF_cmd::UNNOWN):
			sendCommand(DataFrame(DF_cmd::UNNOWN, DF_r::OK, "XD"));
			break;
		case(DF_cmd::SERVER_PING):
			sendCommand(DataFrame(DF_cmd::SERVER_PONG));
			break;
		case(DF_cmd::SERVER_PONG):
			sendCommand(DataFrame(DF_cmd::SERVER_PONG, DF_r::ERR, "Expected 'PING' command"));
			break;
		case(DF_cmd::APP_START):
			std::cout << "Start user app\n";
			app_controler->startApp();
			sendCommand(DataFrame(DF_cmd::APP_START, DF_r::OK));
			break;
		case(DF_cmd::APP_STOP):
			std::cout << "Stop user app\n";
			app_controler->stopApp();
			sendCommand(DataFrame(DF_cmd::APP_STOP, DF_r::OK));
			break;
		case(DF_cmd::APP_PAUSE):
			break;
		case(DF_cmd::APP_RESUME):
			break;
		case(DF_cmd::MODULE_LIST):
			break;
		case(DF_cmd::TAG_GET):
			break;
		case(DF_cmd::TAG_SET):
			break;
		case(DF_cmd::TAG_LIST):
			break;
		case(DF_cmd::FILE_LIST):
			{
				std::error_code err;
				auto result = app_controler->readDir(err);
				if (err) {
					sendCommand(DataFrame(DF_cmd::FILE_LIST, DF_r::ERR, "Cannot Read Files"));
				}
				else {
					auto response = DataFrame(DF_cmd::FILE_LIST, DF_r::OK);
					response.fileList = result;
					sendCommand(response);
				}
			}
			break;
		case(DF_cmd::FILE_READ):
			{
				std::shared_ptr<uint8_t[]> result;
				size_t count;
				bool isOk = app_controler->readFile(data.path, &result, &count);
				if (!isOk) {
					sendCommand(DataFrame(DF_cmd::FILE_READ, DF_r::ERR, "Cannot Read File"));
				}
				else {
					auto response = DataFrame(DF_cmd::FILE_READ, DF_r::OK);
					response.data = std::string(count*2, '0');
					uint8_t* ptr = result.get();
					// convert bytes to hex
					for (size_t i = 0; i < count; i++) {

						uint8_t left = ptr[i] >> 4;
						if (left < 10) response.data[i * 2] = left + '0';
						else response.data[i * 2] = left + 'a' - 0xa;

						uint8_t right = ptr[i] & 0x0f;
						if (right < 10) response.data[i * 2 + 1] = right + '0';
						else response.data[i * 2 + 1] = right + 'a' - 0xa;
					}
					sendCommand(response);
				}
			}
			break;
		case(DF_cmd::FILE_SAVE):
			{
			std::shared_ptr<uint8_t[]> result(new uint8_t[data.data.size()]);
			const char* dataPtr = data.data.c_str();
			uint8_t* ptr = result.get();

			size_t count = data.data.size() / 2;
			for (size_t i = 0; i < count; i++) {

				char c = 0;
				if (dataPtr[i * 2] >= '0' && dataPtr[i * 2] <= '9')       c = (dataPtr[i * 2] - '0') << 4;
				else if(dataPtr[i * 2] >= 'a' && dataPtr[i * 2] <= 'F')   c = (dataPtr[i * 2] - 'a' + 0xa) << 4;
				else if (dataPtr[i * 2] >= 'A' && dataPtr[i * 2] <= 'F')  c = (dataPtr[i * 2] - 'A' + 0xA) << 4;

				if (dataPtr[i * 2 + 1] >= '0' && dataPtr[i * 2 + 1] <= '9')      c += dataPtr[i * 2 + 1] - '0';
				else if (dataPtr[i * 2 + 1] >= 'a' && dataPtr[i * 2 + 1] <= 'F') c += dataPtr[i * 2 + 1] - 'a' + 0xa;
				else if (dataPtr[i * 2 + 1] >= 'A' && dataPtr[i * 2 + 1] <= 'F') c += dataPtr[i * 2 + 1] - 'A' + 0xA;
				ptr[i] = c;
			}
			std::cout << ptr << "\n";
			bool isOk = app_controler->saveFile(data.path, result, count);
			if(isOk) sendCommand(DataFrame(DF_cmd::FILE_SAVE, DF_r::OK));
			else     sendCommand(DataFrame(DF_cmd::FILE_SAVE, DF_r::ERR, "Cannot Save file"));
			}
			break;
		}
	}



	void onWrite(const boost::system::error_code& error, std::size_t bytes_transferred) {

	}


};


// "Attach" connection handle to server class
typedef TCP_server<PLC_server_connection_handle> PLC_TCP_server;



template<typename T>
void tryAssing(boost::json::value const& json, std::string ptr, T& dest)
{
	try {
		std::error_code err;
		auto value = json.find_pointer(ptr, err);
		if (err) return;
		dest = boost::json::value_to<T>(*value);
	}
	catch (...) {
		// ignore exception
		// just end parsing 
	}

};




PLC_server_connection_handle::DataFrame tag_invoke(boost::json::value_to_tag<PLC_server_connection_handle::DataFrame>, boost::json::value const& json) {

	PLC_server_connection_handle::DataFrame frame;
	typedef PLC_server_connection_handle::DataFrame DF;

	std::string cmd = "";
	tryAssing(json, "/Cmd", cmd);
	if (cmd.empty()) return frame;

	if      (cmd == "UNNOWN")       frame.cmd = DF::Command::UNNOWN;
	else if (cmd == "SERVER_PING")  frame.cmd = DF::Command::SERVER_PING;
	else if (cmd == "SERVER_PONG")  frame.cmd = DF::Command::SERVER_PONG;
	else if (cmd == "APP_START")    frame.cmd = DF::Command::APP_START;
	else if (cmd == "APP_STOP")     frame.cmd = DF::Command::APP_STOP;
	else if (cmd == "APP_RESUME")   frame.cmd = DF::Command::APP_RESUME;
	else if (cmd == "APP_PAUSE")    frame.cmd = DF::Command::APP_PAUSE;
	else if (cmd == "MODULE_LIST")  frame.cmd = DF::Command::MODULE_LIST;
	else if (cmd == "TAG_LIST")     frame.cmd = DF::Command::TAG_LIST;
	else if (cmd == "TAG_GET")      frame.cmd = DF::Command::TAG_GET;
	else if (cmd == "TAG_SET")      frame.cmd = DF::Command::TAG_SET;
	else if (cmd == "FILE_LIST")    frame.cmd = DF::Command::FILE_LIST;
	else if (cmd == "FILE_SAVE")    frame.cmd = DF::Command::FILE_SAVE;
	else if (cmd == "FILE_READ")    frame.cmd = DF::Command::FILE_READ;


	tryAssing(json, "/Path", frame.path);
	tryAssing(json, "/Data", frame.data);

	return frame;
}


void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, PLC_server_connection_handle::DataFrame const& frame) {

	typedef PLC_server_connection_handle::DataFrame DF;

	switch (frame.cmd) {
	case DF::Command::UNNOWN:       jv = { { "Cmd", "UNNOWN" } };        break;
	case DF::Command::SERVER_PING:  jv = { { "Cmd", "SERVER_PING" } };   break;
	case DF::Command::SERVER_PONG:  jv = { { "Cmd", "SERVER_PONG" } };   break;
	case DF::Command::APP_START:    jv = { { "Cmd", "APP_START" } };     break;
	case DF::Command::APP_STOP:     jv = { { "Cmd", "APP_STOP" } };      break;
	case DF::Command::APP_RESUME:   jv = { { "Cmd", "APP_RESUME" } };    break;
	case DF::Command::APP_PAUSE:    jv = { { "Cmd", "APP_PAUSE" } };     break;
	case DF::Command::MODULE_LIST:  jv = { { "Cmd", "MODULE_LIST" } };   break;
	case DF::Command::TAG_LIST:     jv = { { "Cmd", "TAG_LIST" } };      break;
	case DF::Command::TAG_GET:      jv = { { "Cmd", "TAG_GET" } };       break;
	case DF::Command::TAG_SET:      jv = { { "Cmd", "TAG_SET" } };       break;
	case DF::Command::FILE_LIST:    jv = { { "Cmd", "FILE_LIST" } };     break;
	case DF::Command::FILE_SAVE:    jv = { { "Cmd", "FILE_SAVE" } };     break;
	case DF::Command::FILE_READ:    jv = { { "Cmd", "FILE_READ" } };     break;
	}

	switch (frame.response) {
	case DF::Response::NONE: break;
	case DF::Response::OK:          jv.as_object().insert({{ "Response", "OK"}}); break;
	case DF::Response::ERR:         jv.as_object().insert({{ "Response", "ERR"}}); break;
	}

	if(!frame.msg.empty())          jv.as_object().insert({ { "Msg", frame.msg} });

	if (frame.cmd == DF::Command::FILE_LIST) {
		jv.as_object().insert({ { "Files", frame.fileList} });
	}

	if (frame.cmd == DF::Command::FILE_READ) {
		if (frame.response == DF::Response::OK)
			jv.as_object().insert({ {"Data", frame.data} });
	}

}
