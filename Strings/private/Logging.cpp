
#include "Logging.hpp"
#include "Allocators.hpp"
#include "TextEditor.hpp"

#include <cstdio>

namespace tp {

	Logger* gLogger = nullptr;

	ualni Logger::Report::getLineCount() const {
		return (ualni)(mLineOffsets.size() - 1);
	}

	String Logger::Report::getString() const {
		return mData;
	}

	void Logger::Report::calcLineCount() {
		Input input = { mData.raw(), (ualni) mData.size() };
		input.getLineOffsets(mLineOffsets);
	}

	Logger::Report::Report() {
		mData = " - ";
	}

	Logger::Report::Report(const String& text) : mData(text) {
		calcLineCount();
	}

	Logger::Report::Report(const String& text, Type type) : mType(type), mData(text) {
		calcLineCount();
	}

	void Logger::write(const String& in, bool post, Report::Type type) {
		StringTemplate copy = in;
		copy.capture();
		mBuff.pushBack(Report(copy, type));

		mLineCount += mBuff.last()->data.getLineCount();

		if (!mCursor) {
			mCursor = mBuff.last();
		}

		if (post) {
			printf("%s", in.raw());
		}
	}

	String Logger::read() {
		if (!mCursor) return {};
    const Report& out = mCursor->data;
    mCursor = mCursor->next;
    return out.getString();
	}

	void Logger::initializeGlobal() {
		DEBUG_ASSERT(!gLogger)
		gLogger = new Logger();
	}

	void Logger::deinitializeGlobal() {
		DEBUG_ASSERT(gLogger)
		delete gLogger;
	}
}