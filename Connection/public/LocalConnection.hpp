#pragma once

#include "ConnectionCommon.hpp"

namespace tp {
	class LocalConnectionContext;

	class LocalConnection : public Connection {
	public:
		class Location {
			String mLocation;

		public:
			Location() :
				mLocation("tmp"){};
			explicit Location(const String& location) :
				mLocation(location) {}
			void setLocation(const String& location) { mLocation = location; }
			[[nodiscard]] const String& getLocation() const { return mLocation; }
			[[nodiscard]] bool exists() const;
		};

	public:
		LocalConnection(){ MODULE_SANITY_CHECK(gModuleConnection) };

		virtual ~LocalConnection() {
			if (mStatus.isOpened()) LocalConnection::disconnect();
		}

	public:
		virtual bool connect(const Location& location, const Type& connectionInfo);
		virtual bool disconnect();

	public:
		virtual const Status& getConnectionStatus() { return mStatus; }
		virtual const Type& getConnectionType() { return mConnectionType; }
		virtual const Location& getLocation() { return mLocation; }

	public:
		virtual bool setPointer(BytePointer pointer);
		virtual bool readBytes(Byte* bytes, SizeBytes size);
		virtual bool writeBytes(const Byte* bytes, SizeBytes size);

	public:
		virtual SizeBytes size();

	private:
		LocalConnectionContext* mHandle = nullptr;
		Location mLocation;
		BytePointer mPointer = 0;
	};
}