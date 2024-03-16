#pragma once

#include "Module.hpp"

namespace tp {
	class ServerContext;
	class ClientContext;

	class Server {
		ServerContext* mContext;

	public:
		typedef void* Socket;

	public:
		Server();
		~Server();

		void start(ualni port);
		Socket accept();
		bool read(Socket client, int1* message, halni size);
		bool write(Socket client, const int1* message, halni size);
	};

	class Client {
		ClientContext* mContext;

	public:
		Client();
		~Client();

		bool connect(const int1* IP, ualni PORT);
		bool read(int1* buff, halni size);
		bool write(const int1* message, halni size);
	};
}