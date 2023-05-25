
#include "StackTrace.hpp"
#include "PrivateConfig.hpp"

#include <Windows.h>
#include <DbgHelp.h>
#include <iostream>
#include <cstdint>

#pragma comment(lib, "dbghelp.lib")

using namespace tp;

tp::CallStackSnapshots tp::gCallStackSnapshots;

// -----------------------  Dict ----------------------- // 

CallStackSnapshots::SnapshotsDict::SnapshotsDict() {
	mTable = (StackShapshot*)malloc(sizeof(StackShapshot) * mSize);
	memsetv(mTable, sizeof(StackShapshot) * mSize, 0);
}

CallStackSnapshots::StackShapshot CallStackSnapshots::SnapshotsDict::getSlot(ualni aIdx) {
	return mTable[aIdx];
}

alni CallStackSnapshots::SnapshotsDict::presents(StackShapshot aPtr) {
	return findSlotRead(aPtr); 
}

void CallStackSnapshots::SnapshotsDict::put(StackShapshot aPtr) {
	auto idx = findSlotWrite(aPtr);
	mTable[idx] = aPtr;
	mEntries++;

	if ((halnf)mEntries / mSize > 2.f / 3.f) {
		resize();
	}
}

void CallStackSnapshots::SnapshotsDict::resize() {
	alni nslots_old = mSize;
	auto table_old = mTable;

	mSize *= 2;
	mTable = (StackShapshot*)malloc(sizeof(StackShapshot) * mSize);
	memsetv(mTable, sizeof(StackShapshot) * mSize, 0);
	mEntries = 0;

	for (alni i = 0; i < nslots_old; i++) {
		if (!table_old[i]) {
			continue;
		}

		alni idx = findSlotWrite(table_old[i]);
		mTable[idx] = table_old[i];
		mEntries++;
	}

	delete[] table_old;
}

alni CallStackSnapshots::SnapshotsDict::findSlotRead(StackShapshot key) {
	ualni const hased_key = hash(key);
	ualni const mask = mSize - 1;
	ualni const shift = (hased_key >> 5) & ~1;
	alni idx = hased_key & mask;
NEXT:
	if (!mTable[idx]) {
		return -1;
	}
	if (compare(mTable[idx], key)) {
		return idx;
	}
SKIP:
	idx = ((5 * idx) + 1 + shift) & mask;
	goto NEXT;
}

ualni CallStackSnapshots::SnapshotsDict::findSlotWrite(StackShapshot key) {
	ualni const hased_key = hash(key);
	ualni const mask = mSize - 1;
	ualni const shift = (hased_key >> 5) & ~1;
	ualni idx = hased_key & mask;
NEXT:
	if (!mTable[idx]) {
		return idx;
	}
	idx = ((5 * idx) + 1 + shift) & mask;
	goto NEXT;
}

CallStackSnapshots::StackShapshot CallStackSnapshots::SnapshotsDict::newStackSnapshot(ualni aDepth) {
	auto const size = sizeof(FramePointer) * (aDepth + 1);
	auto out = (StackShapshot*)malloc(size);
	memsetv(out, size, 0);
}

ualni CallStackSnapshots::SnapshotsDict::hash(StackShapshot snapshot) {
	ualni out = 0;
	for (FramePointer* iter = snapshot; iter; iter++) { out += *iter; }
	return out;
}

bool CallStackSnapshots::SnapshotsDict::compare(StackShapshot left, StackShapshot right) {
	FramePointer* iter_left = left;
	FramePointer* iter_right = right;
	do {
		if (*iter_left != *iter_right) {
			return false;
		}
		iter_left++;
		iter_right++;
	} while (iter_left && iter_right);
	if (*iter_left != *iter_right) {
		return false;
	}
	return true;
}

CallStackSnapshots::SnapshotsDict::~SnapshotsDict() {
	for (ualni idx = 0; idx < mSize; idx++) {
		if (mTable[idx]) {
			free(mTable[idx]);
		}
	}
	free(mTable);
}

// -----------------------  CallStackSnapshots ----------------------- // 

CallStackSnapshots::StackShapshot CallStackSnapshots::getStack(ualni& len) {
	enum { MAX_DEPTH = MEM_STACK_TRACE_MAX_DEPTH };
	static FramePointer pointers[MAX_DEPTH];
	len = 0;

  CONTEXT context;
  RtlCaptureContext(&context);

  STACKFRAME64 stackFrame;
  ZeroMemory(&stackFrame, sizeof(STACKFRAME64));
  stackFrame.AddrPC.Mode = AddrModeFlat;
  stackFrame.AddrFrame.Mode = AddrModeFlat;
  stackFrame.AddrStack.Mode = AddrModeFlat;
  stackFrame.AddrPC.Offset = context.Rip;
  stackFrame.AddrFrame.Offset = context.Rbp;
  stackFrame.AddrStack.Offset = context.Rsp;

  HANDLE processHandle = GetCurrentProcess();
  HANDLE threadHandle = GetCurrentThread();

  while (
		len < MAX_DEPTH &&
		StackWalk64(IMAGE_FILE_MACHINE_AMD64, processHandle, threadHandle, &stackFrame, &context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)
	)
	{
		pointers[len] = stackFrame.AddrFrame.Offset;
		len++;
  }

	return pointers;
}

CallStackSnapshots::StackShapshot CallStackSnapshots::capture() {
	ualni len;
	auto stack = getStack(len);
	auto idx = mSnapshots.presents(stack);

	if (idx == -1) {
		auto new_snapshot = mSnapshots.newStackSnapshot(len + 1);
		memcp(new_snapshot, stack, len * sizeof(FramePointer));
		new_snapshot[len] = 0;
		mSnapshots.put(new_snapshot);
		return new_snapshot;
	}
	
	return mSnapshots.getSlot(idx);
}

void CallStackSnapshots::saveToFile(StackShapshot* snapshots, const char* filepath) {

}

CallStackSnapshots::StackShapshot* CallStackSnapshots::loadFromFile(const char* filepath) {

}