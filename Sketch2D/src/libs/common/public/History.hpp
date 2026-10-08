#pragma once

#include "Environment.hpp"

#include <string>
#include <utility>
#include <vector>

namespace tp::sketch2d {

	template <typename State>
	class History {
	public:
		static constexpr int4 kMinimumLimit = 2;
		static constexpr int4 kMaximumLimit = 1000;
		static constexpr int4 kDefaultLimit = 30;

		struct Entry {
			std::string label;
			std::string mergeKey;
			State state;
		};

		void reset(std::string label, State state) {
			mRevision++;
			mEntries.clear();
			mEntryRevisions.clear();
			mEntries.push_back({ std::move(label), {}, std::move(state) });
			mEntryRevisions.push_back(++mNextEntry);
			mCurrent = 0;
		}

		void commit(std::string label, State state, std::string mergeKey = {}) {
			mRevision++;
			if (!mergeKey.empty() && !mEntries.empty() && mEntries[mCurrent].mergeKey == mergeKey && !canRedo()) {
				mEntries[mCurrent].state = std::move(state);
				if (mEntryRevisions.size() == mEntries.size()) mEntryRevisions[mCurrent] = ++mNextEntry;
				return;
			}

			mEntries.erase(mEntries.begin() + (std::ptrdiff_t) mCurrent + 1, mEntries.end());
			if (mEntryRevisions.size() > mCurrent + 1)
				mEntryRevisions.erase(mEntryRevisions.begin() + (std::ptrdiff_t) mCurrent + 1, mEntryRevisions.end());
			mEntries.push_back({ std::move(label), std::move(mergeKey), std::move(state) });
			while (mEntryRevisions.size() + 1 < mEntries.size())
				mEntryRevisions.push_back(++mNextEntry);
			mEntryRevisions.push_back(++mNextEntry);
			mCurrent = mEntries.size() - 1;
			trim();
		}

		[[nodiscard]] bool canUndo() const { return mCurrent > 0; }
		[[nodiscard]] bool canRedo() const { return mCurrent + 1 < mEntries.size(); }

		const State* undo() { return canUndo() ? jumpTo(mCurrent - 1) : nullptr; }
		const State* redo() { return canRedo() ? jumpTo(mCurrent + 1) : nullptr; }

		const State* jumpTo(size_t index) {
			if (index >= mEntries.size()) return nullptr;
			mCurrent = index;
			mRevision++;
			return &mEntries[mCurrent].state;
		}

		void clear() {
			if (mEntries.empty()) return;
			Entry current = std::move(mEntries[mCurrent]);
			const ualni revision = currentEntryRevision();
			current.mergeKey.clear();
			mEntries.clear();
			mEntryRevisions.clear();
			mEntries.push_back(std::move(current));
			mEntryRevisions.push_back(revision ? revision : ++mNextEntry);
			mCurrent = 0;
		}

		void setLimit(int4 limit) {
			mLimit = limit < kMinimumLimit ? kMinimumLimit : (limit > kMaximumLimit ? kMaximumLimit : limit);
			trim();
		}

		[[nodiscard]] int4 limit() const { return mLimit; }
		[[nodiscard]] size_t currentIndex() const { return mCurrent; }
		[[nodiscard]] ualni revision() const { return mRevision; }
		[[nodiscard]] const std::vector<Entry>& entries() const { return mEntries; }
		[[nodiscard]] const Entry& current() const { return mEntries[mCurrent]; }
		[[nodiscard]] Entry* entryOfRevision(ualni revision) {
			for (size_t index = 0; index < mEntries.size(); index++)
				if (mEntryRevisions.size() == mEntries.size() && mEntryRevisions[index] == revision) return &mEntries[index];
			return nullptr;
		}
		template <typename Fn>
		void forEachEntryFrom(ualni revision, Fn&& fn) {
			if (mEntryRevisions.size() != mEntries.size()) return;
			bool found = false;
			for (size_t index = 0; index < mEntries.size(); index++) {
				found = found || mEntryRevisions[index] == revision;
				if (found) fn(mEntries[index]);
			}
		}
		[[nodiscard]] ualni currentEntryRevision() const {
			return mEntryRevisions.size() == mEntries.size() ? mEntryRevisions[mCurrent] : 0;
		}

		[[nodiscard]] const std::string& undoLabel() const { return mEntries[mCurrent].label; }
		[[nodiscard]] const std::string& redoLabel() const { return mEntries[mCurrent + 1].label; }

	private:
		void trim() {
			const size_t capacity = (size_t) mLimit + 1;
			while (mEntries.size() > capacity && mCurrent > 0) {
				mEntries.erase(mEntries.begin());
				if (!mEntryRevisions.empty()) mEntryRevisions.erase(mEntryRevisions.begin());
				mCurrent--;
			}
			while (mEntries.size() > capacity) {
				mEntries.pop_back();
				if (mEntryRevisions.size() > mEntries.size()) mEntryRevisions.pop_back();
			}
		}

		std::vector<Entry> mEntries;
		size_t mCurrent = 0;
		int4 mLimit = kDefaultLimit;
		ualni mRevision = 0;
		std::vector<ualni> mEntryRevisions;
		ualni mNextEntry = 0;
	};
}
