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

Server::~Server() {
	delete mContext;
	assert(0);
}

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

int1* Server::read(Socket client) {
	short messageSize;
	if (asio::read(*(ServerContext::Socket*) client, asio::buffer(&messageSize, 2)) == -1) {
		std::cerr << "Failed to read from socket" << std::endl;
		((ServerContext::Socket*) client)->close();
		return nullptr;
	}
	auto message = new char[messageSize + 1];
	message[messageSize] = '\0';
	if (asio::read(*(ServerContext::Socket*) client, asio::buffer(message, messageSize)) == -1) {
		std::cerr << "Failed to read from socket" << std::endl;
		memcpy(message, "Client wanna say something but i cant read", 100);
	}
	return message;
}

void Server::write(Socket client, const char* message) {
	auto messageSize = (short) strlen(message);
	if (asio::write(*(ServerContext::Socket*) client, asio::buffer(&messageSize, 2)) == -1) {
		std::cerr << "Failed to write to socket" << std::endl;
	}
	if (asio::write(*(ServerContext::Socket*) client, asio::buffer(message, messageSize)) == -1) {
		std::cerr << "Failed to write to socket" << std::endl;
	}
}

// --------------------------------------------------------------------------------------- //

Client::Client() { mContext = new ClientContext(); }

Client::~Client() {
	delete mContext;
	assert(0);
}

void Client::connect(const char* IP, ualni PORT) {
	asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(IP), PORT);
	mContext->socket.connect(endpoint);
}

char* Client::read() {
	short messageSize;
	if (asio::read(mContext->socket, asio::buffer(&messageSize, 2)) == -1) {
		std::cerr << "Failed to read from socket" << std::endl;
		return nullptr;
	}
	auto message = new char[messageSize + 1];
	message[messageSize] = '\0';
	if (asio::read(mContext->socket, asio::buffer(message, messageSize)) == -1) {
		std::cerr << "Failed to read from socket" << std::endl;
		memcpy(message, "Cant wanna say something but i cant read", 100);
	}
	return message;
}

void Client::write(const char* message) {
	auto messageSize = (short) strlen(message);
	if (asio::write(mContext->socket, asio::buffer(&messageSize, 2)) == -1) {
		std::cerr << "Failed to write to socket" << std::endl;
	}
	if (asio::write(mContext->socket, asio::buffer(message, messageSize)) == -1) {
		std::cerr << "Failed to write to socket" << std::endl;
	}
}
