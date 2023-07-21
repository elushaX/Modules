#pragma once

#include "ConnectionCommon.hpp"

namespace tp {

	class LocalConnectionContext;

	class LocalConnectionLocation {
		String mLocation;
	public:
		LocalConnectionLocation() : mLocation("tmp") {};
		explicit LocalConnectionLocation(const String& location) : mLocation(location) {}
		void setLocation(const String& location) { mLocation = location; }
		[[nodiscard]] const String& getLocation() const { return mLocation; }
		[[nodiscard]] bool exists() const;
	};

	class LocalConnectionType {
	public:
		enum HandleType {
			READ,
			WRITE,
			NONE,
		};

	private:
		HandleType mHandleType;

	public:
		LocalConnectionType() : mHandleType(NONE) {}
		explicit LocalConnectionType(bool read) : mHandleType((HandleType) read) {}
		explicit LocalConnectionType(HandleType handleType) : mHandleType(handleType) {}
		[[nodiscard]] HandleType getType() const { return mHandleType; }
		[[nodiscard]] bool isRead() const { return mHandleType == READ; }
		[[nodiscard]] bool isWrite() const { return mHandleType == WRITE; }
	};

	class LocalConnectionStatus {
	public:
		enum Status {
			NONE,
			OPENED,
			CLOSED,
			DENIED,
			INVALID,
		};

	private:
		Status mStatus = NONE;

	public:
		LocalConnectionStatus() = default;
		[[nodiscard]] Status getStatus() const { return mStatus; }
		void setStatus(Status status) { mStatus = status; }
		[[nodiscard]] bool isOpened() const { return mStatus == OPENED; }
	};

	class LocalConnection {
		typedef ualni SizeBytes;
		typedef ualni BytePointer;
		typedef int1 Byte;

	private:
		LocalConnectionContext* mHandle = nullptr;

		LocalConnectionLocation mLocation;
		LocalConnectionType mConnectionType;
		LocalConnectionStatus mStatus;

		BytePointer mPointer = 0;

	public:
		LocalConnection() {
			MODULE_SANITY_CHECK(gModuleConnection)
		};

		virtual ~LocalConnection() {
			if (mStatus.isOpened()) LocalConnection::disconnect();
		}

	public:
		virtual bool connect(const LocalConnectionLocation& location, const LocalConnectionType& connectionInfo);
		virtual bool disconnect();

	public:
		virtual const LocalConnectionStatus& getConnectionStatus() { return mStatus; }
		virtual const LocalConnectionType& getConnectionType() { return mConnectionType; }
		virtual const LocalConnectionLocation& getLocation() { return mLocation; }

	public:
		virtual bool setPointer(BytePointer pointer);
		virtual bool readBytes(Byte* bytes, SizeBytes size);
		virtual bool writeBytes(const Byte* bytes, SizeBytes size);

	public:
		virtual SizeBytes size();
	};
}