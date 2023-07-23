#pragma once

#include "Environment.hpp"
#include "Map.hpp"

#define MAX_CALL_DEPTH_CAPTURE 128
#define MAX_CALL_CAPTURES_MEM_SIZE_MB 16
#define MAX_DEBUG_INFO_LEN 63
#define FRAMES_TO_SKIP_START 2
#define FRAMES_TO_SKIP_END 3

namespace tp {

	class CallStackCapture {
	public:
		typedef tp::alni FramePointer;

		class CallStack {
			friend CallStackCapture;
			FramePointer frames[MAX_CALL_DEPTH_CAPTURE];

		public:
			[[nodiscard]] ualni getDepth() const;

			class Iterator {
				const FramePointer* mFrame;
			public:
				explicit Iterator(const FramePointer* frame) : mFrame(frame) {};
				FramePointer getFrame() { return *mFrame; }
				bool operator==(const Iterator& in) const { return in.mFrame == mFrame; }
				void operator++() { mFrame++; }
				const Iterator& operator*() const { return *this; }
			};

			[[nodiscard]] Iterator begin() const { return Iterator(frames + FRAMES_TO_SKIP_START); }
			[[nodiscard]] Iterator end() const { return Iterator(frames + FRAMES_TO_SKIP_START + getDepth()); }
		};

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

		static void printSnapshot(const CallStack* snapshot);
		void logAll();

	public:

		template<class Saver>
		void write(Saver& file) {
			file.write(mBuffLoad);
			for (auto cs : *this) {
				file.write(cs.getCallStack()->getDepth());
				for (auto frame : *cs.getCallStack()) {
					file.write((ualni) frame.getFrame());
				}
			}
			file.write(mSymbols);
		}

		// independent of the configuration
		template<class Loader>
		void read(Loader& file) {
			clear();
			ualni loadLen;
			file.read(loadLen);
			for (auto cs = loadLen; cs; cs--) {
				ualni callStackLen;
				file.read(callStackLen);
				for (auto fp = callStackLen; fp; fp--) {
					// --
				}
			}
		}

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
		[[nodiscard]] Iterator end() const { return  Iterator(mBuff + mBuffLoad); }

	private:

		struct CallStackKey {
			CallStack* cs;
			bool operator==(const CallStackKey& in) const;
		};

		static void platformWriteStackTrace(CallStack* stack);
		static void platformWriteDebugSymbols(FramePointer frame, DebugSymbols* out);
		[[nodiscard]] static ualni hashCallStack(CallStackKey key);

		void clear();

	private:
		ualni mBuffLen;
		ualni mBuffLoad;
		CallStack* mBuff;
		Map<CallStackKey, CallStack*, DefaultAllocator, hashCallStack> mSnapshots;
		Map<FramePointer, DebugSymbols> mSymbols;
	};

	extern CallStackCapture* gCSCapture;
}
