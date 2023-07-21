#pragma once

#include "Common.hpp"

namespace tp {
	class FileSystemHandle {
		void* stream;
	public:
		FileSystemHandle();
		~FileSystemHandle();
		void open(const char*, bool);
		bool isOpen();
		void close();
		void seekp(ualni);
		void read(int1*, ualni);
		void write(const int1*, ualni);
		ualni size();
	};


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