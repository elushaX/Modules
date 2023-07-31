#include "NewPlacement.hpp"

#include "LocalConnection.hpp"

#include "bindings/Disk.hpp"

#include <cstdio>

using namespace tp;

bool LocalConnection::Location::exists() const {
	FILE* file = fopen(mLocation.read(), "r");
	if (file) {
		// File exists, close it and return 1
		fclose(file);
		return true;
	}
	return false;
}

bool LocalConnection::connect(const Location& location, const Type& connectionInfo) {
	DEBUG_ASSERT(!mStatus.isOpened());
	if (mStatus.isOpened()) return false;

	auto handle = new LocalConnectionContext();

	switch (connectionInfo.getType()) {
		case Type::READ:
			handle->open(location.getLocation().read(), true);
			break;

		case Type::WRITE:
			handle->open(location.getLocation().read(), false);
			break;

		default:
			break;
	};

	if (!handle->isOpen()) {
		mStatus.setStatus(Status::DENIED);
		delete handle;
		return false;
	}

	mStatus.setStatus(Status::OPENED);
	mHandle = handle;
	mConnectionType = connectionInfo;
	return true;
}

bool LocalConnection::disconnect() {
	DEBUG_ASSERT(mStatus.isOpened());
	if (!mStatus.isOpened() || !mHandle) return false;
	mHandle->close();
	delete mHandle;
	mStatus.setStatus(Status::CLOSED);
	return true;
}

bool LocalConnection::setPointer(BytePointer pointer) {
	DEBUG_ASSERT(mStatus.isOpened());
	if (!mStatus.isOpened()) return false;
	mPointer = pointer;
	return true;
}

bool LocalConnection::readBytes(Byte* bytes, SizeBytes size) {
	DEBUG_ASSERT(mStatus.isOpened() && mConnectionType.isRead());
	if (!mStatus.isOpened() || !mConnectionType.isRead()) return false;

	mHandle->seekp(mPointer);
	mHandle->read(bytes, size);
	mPointer += size;
	return true;
}

bool LocalConnection::writeBytes(const Byte* bytes, SizeBytes size) {
	DEBUG_ASSERT(mStatus.isOpened() && mConnectionType.isWrite());
	if (!mStatus.isOpened() || !mConnectionType.isWrite()) return false;

	mHandle->seekp(mPointer);
	mHandle->write(bytes, size);
	mPointer += size;
	return true;
}

LocalConnection::SizeBytes LocalConnection::size() {
	DEBUG_ASSERT(mStatus.isOpened());
	if (!mStatus.isOpened() || !mHandle) return 0;
	return mHandle->size();
}