#pragma once

#include "Containers.hpp"
#include "Control.hpp"
#include "TextField.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tp::ui {

	struct SearchResult {
		std::string title;
		std::string detail;
		bool enabled = true;
	};

	class SearchField;

	class SearchResultRow : public Control {
	public:
		SearchResultRow(SearchField& field, size_t index, SearchResult result);

		[[nodiscard]] const SearchResult& result() const { return mResult; }
		void setHighlighted(bool highlighted);
		[[nodiscard]] bool isHighlighted() const { return mHighlighted; }

	protected:
		[[nodiscard]] Extent measureContent(Axis axis, halnf crossLength) const override;
		void paint(Painter& painter) override;
		void onActivated() override;
		[[nodiscard]] RGBA backgroundTarget() const override;

	private:
		SearchField& mField;
		size_t mIndex;
		SearchResult mResult;
		bool mHighlighted = false;
	};

	class SearchField : public Widget {
	public:
		static constexpr size_t kDefaultMaximumResults = 8;

		SearchField();
		~SearchField() override;

		void setPlaceholder(std::string text) { mInput.setPlaceholder(std::move(text)); }
		void setMaximumResults(size_t count) { mMaximumResults = count; }
		void setMinimumResultsWidth(halnf width) { mMinimumResultsWidth = width; }

		void setQuery(std::function<std::vector<SearchResult>(const std::string&)> query) { mQuery = std::move(query); }
		void setOnChosen(std::function<void(size_t)> callback) { mOnChosen = std::move(callback); }

		[[nodiscard]] TextField& input() { return mInput; }
		[[nodiscard]] bool isShowingResults() const;
		[[nodiscard]] const std::vector<std::unique_ptr<SearchResultRow>>& rows() const { return mRows; }
		[[nodiscard]] long highlighted() const { return mHighlighted; }

		void setText(const std::string& text);
		void showResults();
		void hideResults();
		void focus();

	protected:
		void filterDescendantEvent(Event& event) override;

	private:
		friend class SearchResultRow;

		void refreshResults();
		void moveHighlight(int step);
		void choose(size_t index);
		[[nodiscard]] long nextEnabled(long from, int step) const;

		TextField mInput;
		Panel mDropdown{ Panel::Surface::Raised };
		std::vector<std::unique_ptr<SearchResultRow>> mRows;
		std::function<std::vector<SearchResult>(const std::string&)> mQuery;
		std::function<void(size_t)> mOnChosen;
		size_t mMaximumResults = kDefaultMaximumResults;
		halnf mMinimumResultsWidth = 0;
		long mHighlighted = -1;
	};
}
