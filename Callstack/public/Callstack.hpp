#pragma once

#include "Environment.hpp"
#include "Map.hpp"

namespace tp {

	class CallStackCapture {
	public:
		typedef tp::alni FramePointer;

		enum {
			MAX_CALL_DEPTH_CAPTURE = 16,
			MAX_CALL_CAPTURES_MEM_SIZE_MB = 32,
			MAX_DEBUG_INFO_LEN = 64,
		};

		class CallStack {
			friend CallStackCapture;
			FramePointer frames[MAX_CALL_DEPTH_CAPTURE];

		public:
			[[nodiscard]] ualni getDepth() const;

			class Iterator {
				const FramePointer* mFrame;

			public:
				explicit Iterator(const FramePointer* frame) : mFrame(frame){};
				FramePointer getFrame() { return *mFrame; }
				bool operator==(const Iterator& in) const { return in.mFrame == mFrame; }
				void operator++() { mFrame++; }
				const Iterator& operator*() const { return *this; }
			};

			[[nodiscard]] Iterator begin() const { return Iterator(frames); }
			[[nodiscard]] Iterator end() const { return Iterator(frames + getDepth()); }
		};

		enum { STACKS_LENGTH = (MAX_CALL_CAPTURES_MEM_SIZE_MB * 1024 * 1024) / sizeof(CallStack) };

		class DebugSymbols {
			friend CallStackCapture;
			char function[MAX_DEBUG_INFO_LEN + 1] = { 0 };
			char file[MAX_DEBUG_INFO_LEN + 1] = { 0 };
			ualni line = 0;

		public:
			[[nodiscard]] const char* getFunc() const { return function; }
			[[nodiscard]] const char* getFile() const { return file; }
			[[nodiscard]] ualni getLine() const { return line; }
		};

	public:
		CallStackCapture();
		~CallStackCapture();

		[[nodiscard]] const CallStack* getSnapshot();
		const DebugSymbols* getSymbols(FramePointer fp);

		void printSnapshot(const CallStack* snapshot);
		void logAll();

	public:
		class Iterator {
			const CallStack* mSnapshot;

		public:
			explicit Iterator(const CallStack* start) : mSnapshot(start) {};
			const CallStack* getCallStack() { return mSnapshot; }
			bool operator==(const Iterator& in) const { return in.mSnapshot == mSnapshot; }
			void operator++() { mSnapshot++; }
			const Iterator& operator*() const { return *this; }
		};

		[[nodiscard]] Iterator begin() const { return Iterator(mBuff); }
		[[nodiscard]] Iterator end() const { return Iterator(mBuff + mBuffLoad); }

	private:
		struct CallStackKey {
			CallStack* cs;
			bool operator==(const CallStackKey& in) const;
		};

		static void platformWriteStackTrace(CallStack* stack);
		static void platformWriteDebugSymbols(FramePointer frame, DebugSymbols* out);
		[[nodiscard]] static ualni hashCallStack(CallStackKey key);
		void platformInit();
		void platformDeinit();

		void clear();

	private:
		CallStack mErrorSnapshot;

		ualni mBuffLen;
		ualni mBuffLoad;
		CallStack* mBuff;

		Map<CallStackKey, CallStack*, DefaultAllocator, hashCallStack> mSnapshots;
		Map<FramePointer, DebugSymbols> mSymbols;
	};
}
