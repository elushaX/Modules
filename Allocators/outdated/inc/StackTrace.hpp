#pragma once

#include "common.h"

namespace tp {

	struct CallStackSnapshots {
		
		typedef alni FramePointer;
		typedef FramePointer* StackShapshot; // NULL Terminated Frames
		
		StackShapshot capture();

		void saveToFile(StackShapshot* snapshots, const char* filepath);
		StackShapshot* loadFromFile(const char* filepath);

	private:

		struct SnapshotsDict {
			SnapshotsDict();
			StackShapshot newStackSnapshot(ualni aDepth);
			void put(StackShapshot aPtr);
			StackShapshot getSlot(ualni aIdx);
			alni presents(StackShapshot aPtr);
			~SnapshotsDict();

		private:
			StackShapshot* mTable = NULL;
			uhalni mSize = 512;
			uhalni mEntries = 0;

			ualni hash(StackShapshot snapshot);
			bool compare(StackShapshot left, StackShapshot right);
			alni findSlotRead(StackShapshot key);
			ualni findSlotWrite(StackShapshot key);
			void resize();
		} mSnapshots;

		StackShapshot getStack(ualni& len);
	};

	extern CallStackSnapshots gCallStackSnapshots;
};
