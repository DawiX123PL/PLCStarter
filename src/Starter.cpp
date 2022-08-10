#include <iostream>
#include "Server.h"





int main() {
	int port = 8000;

	TCP_server server(port);
	server.Start();

	std::cout << "Starting server at port: " << port << "\n";

	getchar(); // this is just to stop program for debugging

	std::cout << "Shutting down server \n";
	server.Stop();
	server.Join();
	std::cout << "Server stopped workign\n";
}