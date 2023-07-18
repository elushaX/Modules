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
}