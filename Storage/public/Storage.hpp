#pragma once

#include "Strings.hpp"

namespace tp {

	extern ModuleManifest gModuleStorage;

	class StorageLocation {
	public:
		StorageLocation() = default;
		virtual ~StorageLocation() = default;
	};

	class ConnectionType {
	public:
		ConnectionType() = default;
		virtual ~ConnectionType() = default;
	};

	class ConnectionStatus {
	public:
		ConnectionStatus() = default;
		virtual ~ConnectionStatus() = default;
	};

	class Storage {
		typedef ualni SizeBytes;
		typedef ualni BytePointer;
		typedef int1 Byte;

	private:
		StorageLocation mLocation;
		ConnectionType mConnectionType;
		ConnectionStatus mStatus;

	public:
		Storage() = default;
		virtual ~Storage() = default;

	public:
		virtual bool connect(const StorageLocation& location, const ConnectionType& connectionInfo) = 0;
		virtual bool disconnect() = 0;
		virtual const ConnectionStatus& getConnectionStatus() = 0;

	public:
		virtual bool readBytes(BytePointer pointer, Byte* bytes, SizeBytes size) = 0;
		virtual bool writeBytes(BytePointer pointer, const Byte* bytes, SizeBytes size) = 0;

	public:
		virtual SizeBytes size() = 0;
	};
};