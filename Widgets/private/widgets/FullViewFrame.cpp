#include "FullViewFrame.hpp"

#include "Event.hpp"
#include "Painter.hpp"
#include "StackLayout.hpp"
#include "Theme.hpp"

#include <algorithm>

using namespace tp;
using namespace tp::ui;

namespace {

	Vec2F mix(Vec2F from, Vec2F to, halnf t) { return from + (to - from) * t; }
}

FullViewFrame::FullViewFrame() {
	setDebugName("full view frame");
	setClipsChildren(true);
	setGeometryAnimated(false);
	setVisible(false);
	emplaceLayout<StackLayout>().setPadding(
		{ kAccentInset + kAccentWidth + kContentPadding, kContentPadding, kContentPadding, kContentPadding }
	);

	mContentSlot.setDebugName("full view content");
	mContentSlot.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mContentSlot.emplaceLayout<StackLayout>();
	mContentSlot.setVisible(false);
	addChild(mContentSlot);

	mPreviewSlot.setDebugName("full view preview");
	mPreviewSlot.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mPreviewSlot.setClipsChildren(true);
	mPreviewSlot.emplaceLayout<StackLayout>();
	addChild(mPreviewSlot);
}

FullViewFrame::~FullViewFrame() {
	setPreview(nullptr);
	setContent(nullptr);
}

void FullViewFrame::setPreview(Widget* preview) {
	if (preview == mPreview) return;
	if (mPreview && mPreview->parent() == &mPreviewSlot) mPreviewSlot.removeChild(*mPreview);
	mPreview = preview;
	if (mPreview) {
		mPreview->setSizing(AxisSizing::fill(), AxisSizing::fill());
		mPreviewSlot.addChild(*mPreview);
	}
	showStage();
}

void FullViewFrame::showStage() {
	const bool full = isContentShown() && mContent;
	mPreviewSlot.setVisible(!full && mPreview);
	mContentSlot.setVisible(full);
}

void FullViewFrame::setContent(Widget* content) {
	if (content == mContent && (!content || content->parent() == &mContentSlot)) return;
	if (mContent && mContent->parent() == &mContentSlot) mContentSlot.removeChild(*mContent);
	mContent = content;
	if (mContent) {
		mContent->setSizing(AxisSizing::fill(), AxisSizing::fill());
		mContentSlot.addChild(*mContent);
	}
	showStage();
}

void FullViewFrame::requestBack() {
	if (mOnBack) mOnBack();
}

void FullViewFrame::setAccent(std::optional<RGBA> accent) {
	mAccent = accent;
	requestPaint();
}

void FullViewFrame::showProgress(halnf progress, bool animated) {
	if (animated) mProgress.animateTo(progress);
	else mProgress.jumpTo(progress);
	startTicking();
	progressChanged();
}

bool FullViewFrame::isContentShown() const { return progress() >= kContentShownProgress; }

RectF FullViewFrame::rectFor(const RectF& item, const RectF& area) const {
	const RectF full = insetRect(area, kMargin);
	const halnf t = std::clamp(progress(), 0.0f, 1.0f);
	return makeRect(mix(item.pos, full.pos, t), mix(item.size, full.size, t));
}

void FullViewFrame::onEvent(Event& event) {
	if (event.type() != Event::Type::KeyDown || event.keyCode() != InputID::ESCAPE) return;
	if (event.target() != this || !mOnBack) return;
	event.accept();
	requestBack();
}

void FullViewFrame::onTick(halnf deltaMs) {
	if (!mProgress.advance(deltaMs)) stopTicking();
	progressChanged();
}

void FullViewFrame::progressChanged() {
	setVisible(progress() > kHiddenProgress || targetProgress() > 0);
	showStage();
	if (parent()) parent()->requestLayout();
	requestPaint();
}

void FullViewFrame::paint(Painter& painter) {
	const Theme& style = theme();
	const halnf radius = style.metrics.cornerRadius;
	painter.fillRect(localRect(), style.palette.raisedSurface, radius);
	if (!mAccent) return;
	const RectF strip = makeRect(kAccentInset, kAccentInset, kAccentWidth, std::max(0.0f, size().y - kAccentInset * 2));
	painter.fillRect(strip, *mAccent, kAccentWidth / 2);
}
