#include "asio.hpp"

#include <iostream>
#include <string>
#include <mutex>

// #include <unistd.h>
// #include <cstring>

constexpr int PORT = 3333;
const char* SERVER_IP = "127.0.0.1";

std::mutex mutex;

void* readServerBroadcast(void* clientSocketPtr) {
	auto socket = ((asio::ip::tcp::socket*)clientSocketPtr);

	while (true) {

		short messageSize;
		asio::read(*socket, asio::buffer(&messageSize, 2));

		mutex.lock();

		auto message = new char[messageSize + 1];
		message[messageSize] = 0;

		asio::read(*socket, asio::buffer(message, messageSize));

		std::cerr << "Broadcast : " << message << std::endl;

		delete[] message;

		mutex.unlock();
	}

	return nullptr;
}

int main() {
	asio::io_context ioContext;

	// Create a TCP socket
	asio::ip::tcp::socket socket(ioContext);

	// Connect to a server
	asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(SERVER_IP), PORT);
	socket.connect(endpoint);

	// Create a new thread to handle the client
	pthread_t threadId;
	if (pthread_create(&threadId, nullptr, readServerBroadcast, (void*) &socket) != 0) {
		std::cerr << "Failed to create thread for client" << std::endl;
		return 1;
	}

	// Detach the thread so it can run independently
	pthread_detach(threadId);

	while (true) {
		std::string message;
		std::cout << " >> ";
		std::cin >> message;

		mutex.lock();

		// Send a message to the server
		auto messageSize = (short) message.size();
		asio::write(socket, asio::buffer(&messageSize, 2));

		// Send a message to the server
		asio::write(socket, asio::buffer(message + "\n"));

		mutex.unlock();
	}

	// Close socket
	// close(clientSocket);

	return 0;
}
