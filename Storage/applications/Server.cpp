#include <asio.hpp>

#include <cstring>
#include <iostream>
#include <list>
#include <mutex>
#include <pthread.h>

class Server {

	struct SharedData {
		std::list<asio::ip::tcp::socket*> clients;
		std::mutex mutex;
	};

	SharedData mSharedData;

	//int serverSocket;
	int port;

public:
	Server(int port) : port(port) {}
	~Server() { assert(0); }

	bool start() {
		// Create socket
		asio::io_context io_context;
		asio::ip::tcp::acceptor serverSocket(io_context);

		// Bind socket to port
		serverSocket.open(asio::ip::tcp::v4());
		// serverSocket.set_option(asio::ip::tcp::acceptor::reuse_address(true));
		serverSocket.bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));

		// Listen for connections
		serverSocket.listen();

		std::cout << "Server listening on port " << port << std::endl;

		// Start accepting clients
		while (true) {
			// Accept client connection
			auto clientSocket = new asio::ip::tcp::socket(io_context);
			serverSocket.accept(*clientSocket);

			// add client
			mSharedData.mutex.lock();
			mSharedData.clients.push_back(clientSocket);
			mSharedData.mutex.unlock();

			// Create a new thread to handle the client
			pthread_t threadId;
			if (pthread_create(&threadId, nullptr, handleClient, this) != 0) {
				std::cerr << "Failed to create thread for client" << std::endl;
				return false;
			}

			// Detach the thread so it can run independently
			pthread_detach(threadId);
		}

		// Close server socket
		serverSocket.close();

		return true;
	}


	static void* handleClient(void* in) {
		auto self = (Server*) in;

		// read shared data - current client id
		self->mSharedData.mutex.lock();
		auto clientSocket = self->mSharedData.clients.back();
		self->mSharedData.mutex.unlock();

	MESSAGE:
		// wait for a message request1
		short messageSize;
		{
			auto bytesRead = asio::read(*clientSocket, asio::buffer(&messageSize, 2));
			if (bytesRead == -1) {
				std::cerr << "Failed to read from socket" << std::endl;
				(*clientSocket).close();
				return nullptr;
			}
		}

		self->mSharedData.mutex.lock();

		// Receive client message
		auto message = new char[messageSize + 1];
		message[messageSize] = '\0';
		{
			auto bytesRead = asio::read(*clientSocket, asio::buffer(message, messageSize));
			if (bytesRead == -1) {
				std::cerr << "Failed to read from socket" << std::endl;
				memcpy(message, "Cant wanna say something but i cant read", 100);
			}
		}

		// Broadcast to all clients
		for (auto client: self->mSharedData.clients) {
			auto bytesWritten = asio::write(*client, asio::buffer(&messageSize, 2));
			if (bytesWritten == -1) {
				std::cerr << "Failed to write to socket" << std::endl;
			}

			bytesWritten = write(*client, asio::buffer(message, messageSize));
			if (bytesWritten == -1) {
				std::cerr << "Failed to write to socket" << std::endl;
			}
		}

		auto exit = memcmp(message, "exit", strlen("exit")) == 0;

		// Close socket
		delete[] message;


		if (exit) {
			(*clientSocket).close();
			self->mSharedData.clients.remove(clientSocket);
		}

		self->mSharedData.mutex.unlock();

		if (exit) {
			return nullptr;
		}
		goto MESSAGE;
	}
};

int main() {
	Server server(3333);
	server.start();

	return 0;
}