#pragma once

#include "Popup.hpp"

#include <functional>
#include <vector>

namespace tp::ui {

	class Widget;

	class PopupStack {
	public:
		struct Entry {
			PopupRequest request;
			RectF rect;
		};

		explicit PopupStack(std::function<void(const PopupRequest&)> onClosed) :
			mOnClosed(std::move(onClosed)) {}

		bool open(const PopupRequest& request);
		void close(const Widget& content);
		void closeTransient();
		void closeFrom(size_t index);
		void closeAll();
		void closeOwnedBy(const Widget& detached);

		[[nodiscard]] bool isOpen(const Widget& content) const { return indexOf(content) >= 0; }
		[[nodiscard]] long indexOf(const Widget& content) const;
		[[nodiscard]] long indexContaining(const Widget& widget) const;
		[[nodiscard]] long topmostModal() const;
		[[nodiscard]] long chainRootOf(const Widget& content) const;

		[[nodiscard]] std::vector<Entry>& entries() { return mEntries; }
		[[nodiscard]] const std::vector<Entry>& entries() const { return mEntries; }
		[[nodiscard]] bool empty() const { return mEntries.empty(); }

	private:
		std::function<void(const PopupRequest&)> mOnClosed;
		std::vector<Entry> mEntries;
		bool mClosingAll = false;
	};
}
