#include "Breadcrumb.hpp"

#include "BoxLayout.hpp"
#include "StackLayout.hpp"
#include "Theme.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kPartGap = 2;
	constexpr const char* kSeparator = "/";
}

BreadcrumbPart::BreadcrumbPart(std::string text, bool current) :
	Button(std::move(text)),
	mCurrent(current) {
	setDebugName("breadcrumb part");
	setStyle(Style::Flat);
	setEnabled(!current);
	if (current) setFontFace(FontFace::Bold);
}

RGBA BreadcrumbPart::labelColor() const { return mCurrent ? theme().palette.text : Button::labelColor(); }

Breadcrumb::Breadcrumb() {
	setDebugName("breadcrumb");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	emplaceLayout<StackLayout>();

	mRow.setSizing(AxisSizing::fit(), AxisSizing::fit());
	mRow.emplaceLayout<RowLayout>(kPartGap).setCrossAlign(CrossAlign::Center);
	mScroll.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mScroll.setShowsScrollBar(false);
	mScroll.setContent(&mRow);
	addChild(mScroll);
}

void Breadcrumb::setParts(const std::vector<std::string>& parts) {
	mRow.removeAllChildren();
	mParts.clear();
	mSeparators.clear();

	for (size_t index = 0; index < parts.size(); index++) {
		if (index > 0) {
			mSeparators.push_back(std::make_unique<Label>(kSeparator));
			mSeparators.back()->setTone(Label::Tone::Muted);
			mRow.addChild(*mSeparators.back());
		}

		const bool current = index + 1 == parts.size();
		mParts.push_back(std::make_unique<BreadcrumbPart>(parts[index], current));
		BreadcrumbPart& part = *mParts.back();
		part.setOnActivate([this, index]() {
			if (mOnPartChosen) mOnPartChosen(index);
		});
		mRow.addChild(part);
	}
	requestLayout();
}
