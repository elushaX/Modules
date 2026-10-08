#include "PopupStack.hpp"

#include "Widget.hpp"

using namespace tp;
using namespace tp::ui;

void PopupStack::closeAll() {
	mClosingAll = true;
	closeFrom(0);
	mClosingAll = false;
}

bool PopupStack::open(const PopupRequest& request) {
	if (mClosingAll || !request.content) return false;

	if (isOpen(*request.content)) close(*request.content);
	closeTransient();

	if (request.options.interactive) {
		const long ownerPopup = request.owner ? indexContaining(*request.owner) : -1;
		closeFrom(ownerPopup + 1);
	}

	mEntries.push_back({ request, makeRect(0, 0, 0, 0) });
	return true;
}

void PopupStack::closeTransient() {
	if (!mEntries.empty() && !mEntries.back().request.options.interactive) closeFrom(mEntries.size() - 1);
}

void PopupStack::close(const Widget& content) {
	const long index = indexOf(content);
	if (index >= 0) closeFrom((size_t) index);
}

void PopupStack::closeOwnedBy(const Widget& detached) {
	for (size_t index = 0; index < mEntries.size(); index++) {
		const PopupRequest& request = mEntries[index].request;
		const bool ownerDetached = request.owner && detached.isSelfOrAncestorOf(*request.owner);
		const bool contentDetached = request.content == &detached;

		if (ownerDetached || contentDetached) {
			closeFrom(index);
			return;
		}
	}
}

long PopupStack::indexOf(const Widget& content) const {
	for (size_t index = 0; index < mEntries.size(); index++) {
		if (mEntries[index].request.content == &content) return (long) index;
	}
	return -1;
}

long PopupStack::indexContaining(const Widget& widget) const {
	for (long index = (long) mEntries.size() - 1; index >= 0; index--) {
		if (mEntries[index].request.content->isSelfOrAncestorOf(widget)) return index;
	}
	return -1;
}

long PopupStack::topmostModal() const {
	for (long index = (long) mEntries.size() - 1; index >= 0; index--) {
		if (mEntries[index].request.options.modal) return index;
	}
	return -1;
}

long PopupStack::chainRootOf(const Widget& content) const {
	long index = indexOf(content);

	while (index > 0) {
		const Widget* owner = mEntries[(size_t) index].request.owner;
		const long ownerPopup = owner ? indexContaining(*owner) : -1;
		if (ownerPopup < 0 || ownerPopup >= index) break;
		index = ownerPopup;
	}

	return index;
}

void PopupStack::closeFrom(size_t index) {
	if (index >= mEntries.size()) return;

	std::vector<Entry> closing(mEntries.begin() + (ptrdiff_t) index, mEntries.end());
	mEntries.erase(mEntries.begin() + (ptrdiff_t) index, mEntries.end());
	for (auto entry = closing.rbegin(); entry != closing.rend(); ++entry)
		mOnClosed(entry->request);
	for (auto entry = closing.rbegin(); entry != closing.rend(); ++entry) {
		if (entry->request.options.onClosed) entry->request.options.onClosed();
	}
}
