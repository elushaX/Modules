#pragma once

#include "ObjectArchiver.hpp"
#include "LocalConnection.hpp"
#include "SizeCounter.hpp"

namespace tp::obj {
	template <bool tRead>
	class Archiver : public ArchiverTemplate<tRead> {
		LocalConnection mConnection;
		ualni mAddress = 0;
		ualni mFirstNotWritten = 0;

	public:
		Archiver() = default;
		explicit Archiver(const char* location) {
			mConnection.connect(LocalConnection::Location(location), LocalConnection::Type(tRead));
		};

		void writeBytes(const int1* val, ualni size) override {
			mConnection.setPointer(mAddress);
			mConnection.writeBytes(val, size);
			incrementAddresses(size);
		}

		void readBytes(int1* val, ualni size) override {
			mConnection.setPointer(mAddress);
			mConnection.readBytes(val, size);
			incrementAddresses(size);
		}

		ualni getAddress() { return mAddress; }

		void setAddress(ualni addr) { mAddress = addr; }

		ualni getFreeAddress() { return mFirstNotWritten; }

		void setFreeAddress(ualni addr) { mFirstNotWritten = addr; }

		bool isOpened() { return mConnection.getConnectionStatus().isOpened(); }

		ualni getSize() { return mConnection.size(); }

	private:
		void incrementAddresses(ualni size) {
			// if (mAddress + size > mFirstNotWritten) mFirstNotWritten = mAddress + size;
			mAddress += size;
		}
	};

	using ArchiverIn = Archiver<true>;
	using ArchiverOut = Archiver<false>;

	void save_string(ArchiverOut& file, const std::string& string);
	ualni save_string_size(const std::string& string);
	void load_string(ArchiverIn& file, std::string& out);

#define SAVE_LOAD_MAX_CALLBACK_SLOTS 100
	typedef void(pre_save_callback)(void* self, ArchiverOut&);
	typedef void(pre_load_callback)(void* self, ArchiverIn&);
	typedef void(post_save_callback)(void* self, ArchiverOut&);
	typedef void(post_load_callback)(void* self, ArchiverIn&);
	typedef alni(slcb_size_callback)(void* self, ArchiverOut&);

	struct save_load_callbacks {
		void* self;
		pre_save_callback* pre_save;
		slcb_size_callback* size;
		pre_load_callback* pre_load;
		post_save_callback* post_save;
		post_load_callback* post_load;
	};
}