#pragma once

#include "ConnectionCommon.hpp"

namespace tp {

	class RemoteConnectionContext;

	class RemoteConnection : public Connection {
	public:
		class Location {
			ualni mId = 0;

		public:
			Location() = default;
		};

	public:
		RemoteConnection() = default;

		virtual ~RemoteConnection() {
			if (mStatus.isOpened()) RemoteConnection::disconnect();
		}

	public:
		virtual bool connect(const Location& location, const Type& connectionInfo);
		virtual bool disconnect();

	public:
		virtual const Location& getLocation() { return mLocation; }

	public:
		virtual bool readBytes(Byte* bytes, SizeBytes size);
		virtual bool writeBytes(const Byte* bytes, SizeBytes size);

	private:
		RemoteConnectionContext* mHandle = nullptr;
		Location mLocation;
	};
}