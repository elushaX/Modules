#pragma once

#include "Common.hpp"

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
		static int1* read(Socket client);
		static void write(Socket client, const int1* message);
	};

	class Client {
		ClientContext* mContext;

	public:
		Client();
		~Client();

		void connect(const int1* IP, ualni PORT);
		int1* read();
		void write(const int1* message);
	};
}