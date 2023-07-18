#pragma once

#include "Storage.hpp"

namespace tp {

	class FileSystemHandle;

	class FileLocation {
		String mLocation;
	public:
		FileLocation() : mLocation("tmp") {};
		explicit  FileLocation(const String& location) : mLocation(location) {}
		void setLocation(const String& location) { mLocation = location; }
		[[nodiscard]] const String& getLocation() const { return mLocation; }
	};

	class FileConnectionType {
	public:
		enum HandleType {
			READ,
			WRITE,
			NONE,
		};

	private:
		HandleType mHandleType;

	public:
		FileConnectionType() : mHandleType(NONE) {}
		explicit FileConnectionType(bool read) : mHandleType((HandleType) read) {}
		explicit FileConnectionType(HandleType handleType) : mHandleType(handleType) {}
		[[nodiscard]] HandleType getType() const { return mHandleType; }
		[[nodiscard]] bool isRead() const { return mHandleType == READ; }
		[[nodiscard]] bool isWrite() const { return mHandleType == WRITE; }
	};

	class FileConnectionStatus {
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
		FileConnectionStatus() = default;
		[[nodiscard]] Status getStatus() const { return mStatus; }
		void setStatus(Status status) { mStatus = status; }
		[[nodiscard]] bool isOpened() const { return mStatus == OPENED; }
	};

	class File {
		typedef ualni SizeBytes;
		typedef ualni BytePointer;
		typedef int1 Byte;

	private:
		FileSystemHandle* mHandle = nullptr;

		FileLocation mLocation;
		FileConnectionType mConnectionType;
		FileConnectionStatus mStatus;

		BytePointer mPointer = 0;

	public:
		File() {
			MODULE_SANITY_CHECK(gModuleStorage)
		};

		virtual ~File() {
			if (mStatus.isOpened()) File::disconnect();
		}

	public:
		virtual bool connect(const FileLocation& location, const FileConnectionType& connectionInfo);
		virtual bool disconnect();

	public:
		virtual const FileConnectionStatus& getConnectionStatus() { return mStatus; }
		virtual const FileConnectionType& getConnectionType() { return mConnectionType; }
		virtual const FileLocation& getLocation() { return mLocation; }

	public:
		virtual bool setPointer(BytePointer pointer);
		virtual bool readBytes(Byte* bytes, SizeBytes size);
		virtual bool writeBytes(const Byte* bytes, SizeBytes size);

	public:
		virtual SizeBytes size();
	};
}