#pragma once

#include "Strings.hpp"

namespace tp {
	extern ModuleManifest gModuleConnection;

	class Connection {
	public:
		typedef ualni SizeBytes;
		typedef ualni BytePointer;
		typedef int1 Byte;

		class Status {
		public:
			enum State {
				NONE,
				OPENED,
				CLOSED,
				DENIED,
				INVALID,
			};

		private:
			State mStatus = NONE;

		public:
			Status() = default;
			[[nodiscard]] State getStatus() const { return mStatus; }
			void setStatus(State status) { mStatus = status; }
			[[nodiscard]] bool isOpened() const { return mStatus == OPENED; }
		};

		class Type {
		public:
			enum State {
				READ,
				WRITE,
				READ_WRITE,
				NONE,
			};

		private:
			State mHandleType = NONE;

		public:
			Type() = default;
			explicit Type(bool read) : mHandleType((State) read) {}
			explicit Type(State handleType) : mHandleType(handleType) {}
			[[nodiscard]] State getType() const { return mHandleType; }
			[[nodiscard]] bool isRead() const { return mHandleType == READ; }
			[[nodiscard]] bool isWrite() const { return mHandleType == WRITE; }
		};

	protected:
		Status mStatus;
		Type mConnectionType;

	public:
		Connection() = default;

		virtual const Status& getConnectionStatus() { return mStatus; }
		virtual const Type& getConnectionType() { return mConnectionType; }
	};
}