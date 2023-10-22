#include "Common.hpp"

namespace tp {
	class LocalConnectionContext {
		void* stream;

	public:
		LocalConnectionContext();
		~LocalConnectionContext();
		void open(const char*, bool);
		bool isOpen();
		void close();
		void seekp(ualni);
		void read(int1*, ualni);
		void write(const int1*, ualni);
		ualni size();
	};
}