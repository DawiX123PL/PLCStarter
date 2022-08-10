#pragma once
#include "Server.h"



class PLC_server_connection_handle : public Server_connection_handle {
public:
	// IMPORTANT - without this constructor code wont compile
	// and compiler will show strange error 
	// NOTE - constructor must be public
	PLC_server_connection_handle(boost::asio::ip::tcp::socket& client) : Server_connection_handle(client) {}



private:

	void onStart() {

	}


	void onRead(const boost::system::error_code& error, size_t bytes_received, uint8_t* data) {
		std::cout << data;
	}


	void onWrite(const boost::system::error_code& error, std::size_t bytes_transferred) {

	}


};


// "Attach" connection handle to server class
typedef TCP_server<PLC_server_connection_handle> PLC_TCP_server;




