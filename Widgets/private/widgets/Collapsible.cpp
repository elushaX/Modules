#include "Collapsible.hpp"

#include "BoxLayout.hpp"
#include "Layout.hpp"
#include "Painter.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

#include <algorithm>
#include <cmath>

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kChevronWidth = 18;
	constexpr halnf kChevronHalf = 3.5f;
	constexpr halnf kPi = 3.14159265f;
}

CollapsibleHeader::CollapsibleHeader(std::string title) :
	mTitle(std::move(title)) {
	setDebugName("collapsible header");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
}

void CollapsibleHeader::setTitle(std::string title) {
	if (title == mTitle) return;
	mTitle = std::move(title);
	requestLayout();
}

void CollapsibleHeader::setSummary(std::string summary) {
	if (summary == mSummary) return;
	mSummary = std::move(summary);
	requestPaint();
}

void CollapsibleHeader::setExpanded(bool expanded, bool animated) {
	const halnf target = expanded ? 1.0f : 0.0f;
	if (animated) animate(mTurn, target);
	else {
		mTurn.jumpTo(target);
		requestPaint();
	}
}

class CollapsibleHeader::HeaderLayout : public Layout {
public:
	explicit HeaderLayout(CollapsibleHeader& header) :
		mHeader(header) {}

protected:
	[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override {
		Widget& action = *mHeader.mAction;
		const Extent own = mHeader.measureContent(axis, crossLength);
		const Extent child = context.measure(action, axis, crossLength);
		if (axis == Axis::Vertical) return Extent::natural(std::max(own.preferred, child.preferred));
		return Extent::shrinkable(own.preferred + child.preferred);
	}

	void arrangeChildren(LayoutContext& context, const RectF& area) override {
		Widget& action = *mHeader.mAction;
		const halnf width = context.measure(action, Axis::Horizontal, area.size.y).preferred;
		const halnf height = std::min(area.size.y, context.measure(action, Axis::Vertical, width).preferred);
		context.place(
			action, makeRect(area.pos.x + area.size.x - width, area.pos.y + (area.size.y - height) / 2, width, height)
		);
	}

private:
	CollapsibleHeader& mHeader;
};

void CollapsibleHeader::setAction(Widget* action) {
	if (mAction) removeChild(*mAction);
	mAction = action;
	if (!action) {
		setLayout(nullptr);
		return;
	}
	addChild(*action);
	setLayout(std::make_unique<HeaderLayout>(*this));
}

halnf CollapsibleHeader::actionWidth() const { return mAction ? mAction->size().x : 0; }

Extent CollapsibleHeader::measureContent(Axis axis, halnf) const {
	const Metrics& metrics = theme().metrics;
	if (axis == Axis::Vertical) return Extent::natural(metrics.controlHeight);
	const halnf summary = mSummary.empty() ? 0 : textWidth(mSummary, metrics.smallFontSize) + 12;
	return Extent::shrinkable(
		kChevronWidth + textWidth(mTitle, metrics.fontSize) + summary + metrics.controlPadding.left
	);
}

RGBA CollapsibleHeader::backgroundTarget() const {
	const Palette& palette = theme().palette;
	if (isPressed()) return palette.controlPressed;
	if (isHovered()) return palette.controlHovered;
	return { palette.controlHovered.r, palette.controlHovered.g, palette.controlHovered.b, 0 };
}

void CollapsibleHeader::paint(Painter& painter) {
	const Theme& style = theme();
	const Metrics& metrics = style.metrics;
	painter.fillRect(localRect(), background(), metrics.cornerRadius);
	if (showsFocusRing())
		painter.strokeRect(insetRect(localRect(), -2), style.palette.focusRing, metrics.cornerRadius + 2, 2);

	const Vec2F center = { kChevronWidth / 2 + 2, size().y / 2 };
	const halnf angle = mTurn.value() * kPi / 2;
	const halnf cosine = std::cos(angle);
	const halnf sine = std::sin(angle);
	auto turned = [&](Vec2F point) {
		return center + Vec2F(point.x * cosine - point.y * sine, point.x * sine + point.y * cosine);
	};
	painter.polyline(
		{ turned(Vec2F(-kChevronHalf / 2, -kChevronHalf)),
			turned(Vec2F(kChevronHalf / 2, 0.0f)),
			turned(Vec2F(-kChevronHalf / 2, kChevronHalf)) },
		style.palette.mutedText,
		1.5f
	);

	const RectF textBox = makeRect(
		kChevronWidth + 4, 0, size().x - kChevronWidth - 4 - metrics.controlPadding.right - actionWidth(), size().y
	);
	painter.text(mTitle, textBox, metrics.fontSize, TextAlign::Start, style.palette.text);
	if (!mSummary.empty())
		painter.text(mSummary, textBox, metrics.smallFontSize, TextAlign::End, style.palette.mutedText);
}

class Collapsible::SectionLayout : public Layout {
public:
	explicit SectionLayout(Collapsible& section) :
		mSection(section) {}

protected:
	[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override {
		const Extent header = context.measure(mSection.mHeader, axis, crossLength);
		if (!mSection.mReveal.isVisible()) return header;

		const Extent body = context.measure(mSection.mBody, axis, crossLength);
		if (axis == Axis::Horizontal) {
			return { std::max(header.minimum, body.minimum), std::max(header.preferred, body.preferred), kUnbounded, 0 };
		}
		return Extent::rigid(header.preferred + revealed(body.preferred));
	}

	void arrangeChildren(LayoutContext& context, const RectF& area) override {
		const halnf headerHeight = context.measure(mSection.mHeader, Axis::Vertical, area.size.x).preferred;
		context.place(mSection.mHeader, makeRect(area.pos.x, area.pos.y, area.size.x, headerHeight));
		if (!mSection.mReveal.isVisible()) return;

		const halnf bodyHeight = context.measure(mSection.mBody, Axis::Vertical, area.size.x).preferred;
		const halnf shown = revealed(bodyHeight);
		context.place(mSection.mReveal, makeRect(area.pos.x, area.pos.y + headerHeight, area.size.x, shown));
	}

private:
	[[nodiscard]] halnf revealed(halnf bodyHeight) const {
		return std::round(bodyHeight * std::clamp(mSection.mOpen.value(), 0.0f, 1.0f));
	}

	Collapsible& mSection;
};

class CollapsibleRevealLayout : public Layout {
public:
	explicit CollapsibleRevealLayout(Widget& body) :
		mBody(body) {}

protected:
	[[nodiscard]] Extent measureChildren(const LayoutContext& context, Axis axis, halnf crossLength) const override {
		return context.measure(mBody, axis, crossLength);
	}

	void arrangeChildren(LayoutContext& context, const RectF& area) override {
		const halnf height = context.measure(mBody, Axis::Vertical, area.size.x).preferred;
		context.place(mBody, makeRect(area.pos.x, area.pos.y + area.size.y - height, area.size.x, height));
	}

private:
	Widget& mBody;
};

Collapsible::Collapsible(std::string title) :
	mHeader(std::move(title)) {
	setDebugName("collapsible");
	setSizing(AxisSizing::fill(), AxisSizing::fit());
	setClipsChildren(true);
	setLayout(std::make_unique<SectionLayout>(*this));
	mHeader.setGeometryAnimated(false);
	setGeometryAnimated(false);

	mBody.setDebugName("collapsible body");
	mBody.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mBody.setGeometryAnimated(false);
	mBody.emplaceLayout<ColumnLayout>(6).setPadding(Insets{ kChevronWidth + 4, 4, 0, 6 });

	mReveal.setDebugName("collapsible reveal");
	mReveal.setSizing(AxisSizing::fill(), AxisSizing::fit());
	mReveal.setGeometryAnimated(false);
	mReveal.setClipsChildren(true);
	mReveal.setLayout(std::make_unique<CollapsibleRevealLayout>(mBody));
	mReveal.addChild(mBody);
	mReveal.setVisible(false);

	mHeader.setOnActivate([this]() { toggle(); });

	addChild(mReveal);
	addChild(mHeader);
}

void Collapsible::setExpanded(bool expanded, bool animated) {
	if (expanded == mExpanded) return;
	mExpanded = expanded;
	mHeader.setExpanded(expanded, animated);

	const halnf target = expanded ? 1.0f : 0.0f;
	if (expanded) mReveal.setVisible(true);
	if (animated && ui()) {
		mOpen.animateTo(target);
		startTicking();
	} else {
		mOpen.jumpTo(target);
		mReveal.setVisible(expanded);
	}
	requestLayout();
	if (mOnToggled) mOnToggled(expanded);
}

void Collapsible::finishTransitions() {
	if (!mOpen.isAnimating()) return;
	mOpen.finish();
	if (!mExpanded) mReveal.setVisible(false);
	requestLayout();
}

void Collapsible::onTick(halnf deltaMs) {
	const bool animating = mOpen.advance(deltaMs);
	if (!animating) {
		stopTicking();
		if (!mExpanded) mReveal.setVisible(false);
	}
	requestLayout();
	requestPaint();
}
