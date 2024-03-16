#pragma once

#include "Archiver.hpp"
#include "ConnectionCommon.hpp"

namespace tp {
	class LocalConnectionContext;

	class LocalConnection : public Connection {
	public:
		class Location {
			std::string mLocation;

		public:
			Location() :
				mLocation("tmp"){};
			explicit Location(const std::string& location) :
				mLocation(location) {}
			void setLocation(const std::string& location) { mLocation = location; }
			[[nodiscard]] const std::string& getLocation() const { return mLocation; }
			[[nodiscard]] bool exists() const;
		};

	public:
		LocalConnection() = default;

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

	template <bool tRead>
	class ArchiverLocalConnection : public ArchiverTemplate<tRead> {
	public:
		ArchiverLocalConnection() = default;

	protected:
		void writeBytes(const int1* val, ualni size) override { connection.writeBytes(val, size); }
		void readBytes(int1* val, ualni size) override { connection.readBytes(val, size); }

	public:
		LocalConnection connection;
	};
}