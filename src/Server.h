#pragma once
#include <boost/asio.hpp>
#include "thread.h"
#include <memory>


// NOTE:
// - server and all connectionHandlers work in single thread, 
//   probably it is safe to use shared varables without mutexes
// 
// 
// 
// almost all of this code is copy/paste from examples and tutorials
// links:
// boost.org:     https://www.boost.org/doc/libs/1_79_0/doc/html/boost_asio/tutorial/tutdaytime3.html
// echo example:  boost_1_79_0\libs\asio\example\cpp11\echo
// chat examplt:  boost_1_79_0\libs\asio\example\cpp11\chat




// this is is base to create custom client connection
class Server_connection_handle : public std::enable_shared_from_this<Server_connection_handle> {
	boost::asio::ip::tcp::socket _client;

	// read buffer 
	static constexpr size_t readBufSize = 1024;
	uint8_t readBuf[readBufSize];

public:


	Server_connection_handle(boost::asio::ip::tcp::socket& client) :
		_client(std::move(client)), readBuf()
	{

	}

	void start() {
		read();
		onStart();
	}


protected:

	virtual void onStart() = 0;

	// this function is called when succesfully(or not) received some data
	virtual void onRead(const boost::system::error_code& error, size_t bytes_received, uint8_t* data) = 0;

	// this function is called when data is succesfuly(or not) sent to client
	virtual void onWrite(const boost::system::error_code& error, std::size_t bytes_transferred) = 0;


	void read() {
		// "self" is just to hold pointer to Server_connection;
		// for some reason it is hard to tell if io_context still holds any reference to this class
		// because of this I have to do this strange thing
		auto self = shared_from_this();

		_client.async_read_some(
			boost::asio::buffer(readBuf, readBufSize),
			[this, self](const boost::system::error_code& error, size_t bytes_received)
			{
				onRead(error, bytes_received, readBuf);
				if (!error) read(); // same comment as in async_accept()
				// NOT recursion
			}
		);
	}
	uint8_t writeBuf[readBufSize];

protected:
	size_t write(const uint8_t* const data, size_t len) {

		// copy data to memory
		// shared pointer is necessary because data must be valid until handler is called
		std::shared_ptr<uint8_t[]> mem(new uint8_t[len]);
		memcpy(mem.get(), data, len);


		// (almost) same as read()
		auto self = shared_from_this();
		_client.async_write_some(
			boost::asio::buffer(mem.get(), len),
			[this, self, mem](const boost::system::error_code& error, std::size_t bytes_transferred)
			{onWrite(error, bytes_transferred); }
		);

		return len;
	}

};




// this class is just to test if everything is working 
class Server_connection_default : public std::enable_shared_from_this<Server_connection_default>, public Server_connection_handle {
public :

	// IMPORTANT - without this constructor code wont compile
	// and compiler will show strange error 
	Server_connection_default(boost::asio::ip::tcp::socket& client) : Server_connection_handle(client){}


private:

	void onStart() {
		std::cout << "Client connected succesfully\n";
		write((const uint8_t* const)"Hello Client", 13);
	}


	// this function is called when succesfully(or not) received some data
	void onRead(const boost::system::error_code& error, size_t bytes_received, uint8_t* data) {
		if (error) {
			std::cout << "ERROR: could not read message: \n\t"
				<< error.value() << " -- "
				<< error.message()
				<< "\n";
		}
		std::cout << "Client message: \n\t";
		for (int i = 0; i < bytes_received; i++)
			std::cout << data[i];
	}

	// this function is called when data is succesfuly(or not) sent to client
	void onWrite(const boost::system::error_code& error, std::size_t bytes_transferred) {
		if (error) {
			std::cout << "ERROR: could not send message :\n\t" 
				<< error.value() << " -- "
				<< error.message()
				<< "\n";
		}
	}

};



// this class handles all connections in separate thread
template <class Client_connection_class>
class TCP_server : public Thread {

	boost::asio::io_context context;
	boost::asio::ip::tcp::acceptor acceptor;

public:
	TCP_server(int port) :
		acceptor(context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::tcp::v4(), port))
	{}

private:

	void threadJob() {
		// allow io_context to accept incomming connection
		accept();
		try {
			// running event loop 
			// this loop handles all communication
			while (IsRun()) context.run_for(std::chrono::milliseconds(100));
		}
		catch (std::exception& e) {
			std::cout << "Caugth error in context.run(): \n\t" << e.what() << "\n";
		}
	}

	void accept() {
		using namespace boost::asio::ip;
		using namespace boost::system;

		// set accept event handler
		acceptor.async_accept
		(
			[this](const error_code& error, tcp::socket client)
			{
				onAccept(error, client); 
				accept(); // after accepted connection we must tell acceptor to wait for new connection
				// this might look like recursion, but this is called in context.run() on incomming connection
			}
		);
	}

	// this is called when connection is succesfully created
	void onAccept(const boost::system::error_code& error, boost::asio::ip::tcp::socket& client) {
		// start connection
		auto connection = std::make_shared<Client_connection_class>(client);
		connection->start();
	}

};



typedef TCP_server<Server_connection_default> TCP_server_default;
