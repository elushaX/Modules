#include "SearchField.hpp"

#include <algorithm>

#include "BoxLayout.hpp"
#include "Painter.hpp"
#include "StackLayout.hpp"
#include "Ui.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kRowPadding = 8;
	constexpr halnf kDetailGap = 2;
}

SearchResultRow::SearchResultRow(SearchField& field, size_t index, SearchResult result) :
	mField(field),
	mIndex(index),
	mResult(std::move(result)) {
	setDebugName("search result: " + mResult.title);
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	setEnabled(mResult.enabled);
}

void SearchResultRow::setHighlighted(bool highlighted) {
	if (highlighted == mHighlighted) return;
	mHighlighted = highlighted;
	requestPaint();
}

Extent SearchResultRow::measureContent(Axis axis, halnf) const {
	const Metrics& metrics = theme().metrics;
	if (axis == Axis::Vertical) {
		halnf height = metrics.lineHeight + kRowPadding;
		if (!mResult.detail.empty()) height += metrics.lineHeight * 0.85f + kDetailGap;
		return Extent::rigid(height);
	}
	const halnf widest =
		std::max(textWidth(mResult.title, metrics.fontSize), textWidth(mResult.detail, metrics.smallFontSize));
	return Extent::natural(widest + kRowPadding * 2);
}

void SearchResultRow::paint(Painter& painter) {
	const Theme& style = theme();
	const Metrics& metrics = style.metrics;
	painter.fillRect(localRect(), background(), metrics.cornerRadius);

	const RGBA titleColor = isEffectivelyEnabled() ? style.palette.text : style.palette.disabledText;
	const halnf width = size().x - kRowPadding * 2;
	painter.text(
		mResult.title,
		makeRect(kRowPadding, kRowPadding / 2, width, metrics.lineHeight),
		metrics.fontSize,
		TextAlign::Start,
		titleColor
	);
	if (mResult.detail.empty()) return;

	painter.text(
		mResult.detail,
		makeRect(kRowPadding, kRowPadding / 2 + metrics.lineHeight + kDetailGap, width, metrics.lineHeight),
		metrics.smallFontSize,
		TextAlign::Start,
		style.palette.mutedText
	);
}

void SearchResultRow::onActivated() { mField.choose(mIndex); }

RGBA SearchResultRow::backgroundTarget() const {
	if (mHighlighted && isEffectivelyEnabled()) return theme().palette.menuHighlight;
	return Control::backgroundTarget();
}

SearchField::SearchField() {
	setDebugName("search field");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	emplaceLayout<StackLayout>();
	addChild(mInput);

	mDropdown.setDebugName("search results");
	mDropdown.setBorder(true);
	mDropdown.emplaceLayout<ColumnLayout>(0).setPadding(Insets::uniform(4));
	mInput.setOnEdited([this](const std::string&) { refreshResults(); });
	mInput.setOnEditingStarted([this]() { refreshResults(); });
}

SearchField::~SearchField() { hideResults(); }

bool SearchField::isShowingResults() const { return ui() && ui()->isPopupOpen(mDropdown); }

void SearchField::setText(const std::string& text) {
	mInput.setText(text);
	refreshResults();
}

void SearchField::focus() {
	if (ui()) ui()->setKeyboardFocus(&mInput);
}

void SearchField::refreshResults() {
	std::vector<SearchResult> results = mQuery ? mQuery(mInput.text()) : std::vector<SearchResult>{};
	if (results.size() > mMaximumResults) results.resize(mMaximumResults);

	mDropdown.removeAllChildren();
	mRows.clear();
	for (size_t index = 0; index < results.size(); index++) {
		mRows.push_back(std::make_unique<SearchResultRow>(*this, index, std::move(results[index])));
		mDropdown.addChild(*mRows.back());
	}
	mHighlighted = nextEnabled(-1, 1);
	for (size_t index = 0; index < mRows.size(); index++)
		mRows[index]->setHighlighted(long(index) == mHighlighted);

	if (mRows.empty()) hideResults();
	else showResults();
}

void SearchField::showResults() {
	if (mRows.empty() || !ui()) return;

	ui()->updateLayout();
	const halnf width = std::max(windowRect().size.x, mMinimumResultsWidth);
	mDropdown.setSizing(AxisSizing::fixed(width), AxisSizing::fit());
	if (isShowingResults()) {
		mDropdown.requestLayout();
		return;
	}

	const RectF field = windowRect();
	PopupRequest request;
	request.content = &mDropdown;
	request.owner = this;
	request.anchor = makeRect(field.pos.x + (field.size.x - width) / 2, field.pos.y, width, field.size.y);
	request.options.placement = Placement::Below;
	request.options.closeOnEscape = false;
	ui()->openPopup(request);
}

void SearchField::hideResults() {
	if (isShowingResults()) ui()->closePopup(mDropdown);
}

long SearchField::nextEnabled(long from, int step) const {
	for (long index = from + step; index >= 0 && index < long(mRows.size()); index += step) {
		if (mRows[size_t(index)]->result().enabled) return index;
	}
	return from >= 0 && from < long(mRows.size()) ? from : -1;
}

void SearchField::moveHighlight(int step) {
	const long next = nextEnabled(mHighlighted, step);
	if (next < 0) return;

	if (mHighlighted >= 0) mRows[size_t(mHighlighted)]->setHighlighted(false);
	mHighlighted = next;
	mRows[size_t(mHighlighted)]->setHighlighted(true);
}

void SearchField::choose(size_t index) {
	if (index >= mRows.size() || !mRows[index]->result().enabled) return;

	hideResults();
	if (mOnChosen) mOnChosen(index);
}

void SearchField::filterDescendantEvent(Event& event) {
	if (event.type() != Event::Type::KeyDown || !isShowingResults()) return;

	switch (event.keyCode()) {
		case InputID::DOWN: moveHighlight(1); break;
		case InputID::UP: moveHighlight(-1); break;
		case InputID::ENTER:
		case InputID::KP_ENTER:
			if (mHighlighted >= 0) choose(size_t(mHighlighted));
			break;
		case InputID::ESCAPE: hideResults(); break;
		default: return;
	}
	event.accept();
}
