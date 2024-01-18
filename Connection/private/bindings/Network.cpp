#include "Network.hpp"

#include "asio.hpp"

#include <iostream>

using namespace tp;

class tp::ServerContext {
	friend class Server;

	asio::io_context context;
	asio::ip::tcp::acceptor socket;
	typedef asio::ip::tcp::socket Socket;

	ServerContext() :
		socket(context) {}

	~ServerContext() {
		context.stop();
		socket.close();
	}
};

class tp::ClientContext {
	friend Client;

	asio::io_context context;
	asio::ip::tcp::socket socket;

public:
	ClientContext() :
		socket(context) {}

	~ClientContext() {
		context.stop();
		socket.close();
	}
};

// --------------------------------------------------------------------------------------- //

Server::Server() { mContext = new ServerContext(); }

Server::~Server() { delete mContext; }

void Server::start(ualni port) {
	mContext->socket.open(asio::ip::tcp::v4());
	mContext->socket.bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port));
	mContext->socket.listen();
	std::cout << "Server listening on port " << port << std::endl;
}

Server::Socket Server::accept() {
	auto clientSocket = new asio::ip::tcp::socket(mContext->context);
	mContext->socket.accept(*clientSocket);
	std::cout << "New client accepted " << std::endl;
	return clientSocket;
}

bool Server::read(Socket client, int1* message, halni messageSize) {
	try {
		std::size_t bytesRead = asio::read(*(ServerContext::Socket*) client, asio::buffer(message, messageSize));
		return bytesRead == messageSize;
	} catch (const std::exception& e) {
		std::cerr << "Error during read: " << e.what() << "\n";
		return false;
	}
}

bool Server::write(Socket client, const int1* message, halni messageSize) {
	try {
		std::size_t bytesWritten = asio::write(*(ServerContext::Socket*) client, asio::buffer(message, messageSize));
		return bytesWritten == messageSize;
	} catch (const std::exception& e) {
		std::cerr << "Error during write: " << e.what() << "\n";
		return false;
	}
}

// --------------------------------------------------------------------------------------- //

Client::Client() { mContext = new ClientContext(); }

Client::~Client() { delete mContext; }

bool Client::connect(const char* IP, ualni PORT) {
	try {
		asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(IP), PORT);
		mContext->socket.connect(endpoint);
		return true;
	} catch (const std::exception& e) {
		std::cerr << "Error during read: " << e.what() << "\n";
		return false;
	}
}

bool Client::read(int1* buff, halni size) {
	try {
		std::size_t bytesRead = asio::read(mContext->socket, asio::buffer(buff, size));
		return bytesRead == size;
	} catch (const std::exception& e) {
		std::cerr << "Error during read: " << e.what() << "\n";
		return false;
	}
}

bool Client::write(const int1* message, halni messageSize) {
	try {
		std::size_t bytesWritten = asio::write(mContext->socket, asio::buffer(message, messageSize));
		return bytesWritten == messageSize;
	} catch (const std::exception& e) {
		std::cerr << "Error during write: " << e.what() << "\n";
		return false;
	}
}
