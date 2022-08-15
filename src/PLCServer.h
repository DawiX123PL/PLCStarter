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
			PING,           // ping device
			PONG,           // response to ping
			START,          // start or resume app
			STOP,           // kill app
			RESUME,         // resume app - error it app is stopped
			PAUSE,          // pause app - error if app is soppped
			GET_IO_MODULES, // list of all modules connected to device
			GET_TAG,        // get tag value
			SET_TAG,        // set tag to value 
			GET_TAG_LIST,   // get list of tags 
		} cmd = Command::UNNOWN;
		
		enum class Response {
			NONE,
			OK,
			ERR
		}response = Response::NONE;

		std::string msg;

		DataFrame(){}
		DataFrame(Command c) :cmd(c): msg("") {}
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
			sendCommand(DataFrame(DF_cmd::UNNOWN, "XD"));
			break;
		case(DF_cmd::PING):
			sendCommand(DataFrame(DF_cmd::PONG));
			break;
		case(DF_cmd::PONG):
			sendCommand(DataFrame(DF_cmd::PONG, DF_r::ERR, "Expected 'PING' command"));
			break;
		case(DF_cmd::START):
			std::cout << "Start user app\n";
			app_controler->startApp();
			sendCommand(DataFrame(DF_cmd::START, DF_r::OK));
			break;
		case(DF_cmd::STOP):
			std::cout << "Stop user app\n";
			app_controler->stopApp();
			sendCommand(DataFrame(DF_cmd::STOP, DF_r::OK));
			break;
		case(DF_cmd::PAUSE):
			break;
		case(DF_cmd::RESUME):
			break;
		case(DF_cmd::GET_IO_MODULES):
			break;
		case(DF_cmd::GET_TAG):
			break;
		case(DF_cmd::SET_TAG):
			break;
		case(DF_cmd::GET_TAG_LIST):
			break;
		}
	}


	void onWrite(const boost::system::error_code& error, std::size_t bytes_transferred) {

	}


};


// "Attach" connection handle to server class
typedef TCP_server<PLC_server_connection_handle> PLC_TCP_server;




PLC_server_connection_handle::DataFrame tag_invoke(boost::json::value_to_tag<PLC_server_connection_handle::DataFrame>, boost::json::value const& json) {

	PLC_server_connection_handle::DataFrame frame;
	typedef PLC_server_connection_handle::DataFrame DF;

	if (!json.at("Cmd").is_string()) return frame;
	auto& cmd = json.at("Cmd").get_string();

	if      (cmd == "UNNOWN")           frame.cmd = DF::Command::UNNOWN;
	else if (cmd == "PING")             frame.cmd = DF::Command::PING;
	else if (cmd == "PONG")             frame.cmd = DF::Command::PONG;
	else if (cmd == "START")            frame.cmd = DF::Command::START;
	else if (cmd == "STOP")             frame.cmd = DF::Command::STOP;
	else if (cmd == "PAUSE")            frame.cmd = DF::Command::PAUSE;
	else if (cmd == "RESUME")           frame.cmd = DF::Command::RESUME;
	else if (cmd == "GET_IO_MODULES")   frame.cmd = DF::Command::GET_IO_MODULES;
	else if (cmd == "GET_TAG")          frame.cmd = DF::Command::GET_TAG;
	else if (cmd == "SET_TAG")          frame.cmd = DF::Command::SET_TAG;
	else if (cmd == "GET_TAG_LIST")     frame.cmd = DF::Command::GET_TAG_LIST;

	return frame;
}


void tag_invoke(boost::json::value_from_tag, boost::json::value& jv, PLC_server_connection_handle::DataFrame const& frame) {

	typedef PLC_server_connection_handle::DataFrame DF;

	switch (frame.cmd) {
	case DF::Command::UNNOWN:          jv = { { "Cmd", "UNNOWN" } };        break;
	case DF::Command::PING:            jv = { { "Cmd", "PING" } };          break;
	case DF::Command::PONG:            jv = { { "Cmd", "PONG" } };          break;
	case DF::Command::START:           jv = { { "Cmd", "START" } };         break;
	case DF::Command::STOP:            jv = { { "Cmd", "STOP" } };          break;
	case DF::Command::RESUME:          jv = { { "Cmd", "RESUME" } };        break;
	case DF::Command::PAUSE:           jv = { { "Cmd", "PAUSE" } };         break;
	case DF::Command::GET_IO_MODULES:  jv = { { "Cmd", "GET_IO_MODULES" } }; break;
	case DF::Command::GET_TAG:         jv = { { "Cmd", "GET_TAG" } };       break;
	case DF::Command::SET_TAG:         jv = { { "Cmd", "SET_TAG" } };       break;
	case DF::Command::GET_TAG_LIST:    jv = { { "Cmd", "GET_TAG_LIST" } };  break;
	}

	switch (frame.response) {
	case DF::Response::NONE: break;
	case DF::Response::OK:          jv.as_object().insert({{ "Response", "OK"}}); break;
	case DF::Response::ERR:         jv.as_object().insert({{ "Response", "ERR"}}); break;
	}

	if(!frame.msg.empty())          jv.as_object().insert({ { "Msg", frame.msg} });

}
