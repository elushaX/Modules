#include "ResizeHandle.hpp"

#include "Event.hpp"
#include "Painter.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kLineThickness = 2;
	constexpr halnf kLineInset = 8;
}

ResizeHandle::ResizeHandle(Axis dragAxis) :
	mDragAxis(dragAxis) {
	setDebugName("resize handle");
	setGeometryAnimated(false);
}

RectF ResizeHandle::lineRect() const {
	const Axis lengthAxis = crossAxis(mDragAxis);
	const halnf available = lengthAlong(localRect(), lengthAxis);
	const halnf inset = available > kLineInset * 4 ? kLineInset : 0;

	RectF line;
	setSpanAlong(line, mDragAxis, (lengthAlong(localRect(), mDragAxis) - kLineThickness) / 2, kLineThickness);
	setSpanAlong(line, lengthAxis, inset, available - inset * 2);
	return line;
}

void ResizeHandle::onEvent(Event& event) {
	switch (event.type()) {
		case Event::Type::PointerDown:
			if (event.button() != PointerButton::Primary) return;
			mDragging = true;
			mGrabOffset = along(event.position(), mDragAxis) - lengthAlong(localRect(), mDragAxis) / 2;
			ui()->capturePointer(*this);
			if (mOnStarted) mOnStarted();
			break;

		case Event::Type::PointerMove:
			if (!mDragging) return;
			if (mOnDragged) mOnDragged(along(event.windowPosition(), mDragAxis) - mGrabOffset);
			break;

		case Event::Type::PointerUp:
			if (!mDragging || event.button() != PointerButton::Primary) return;
			mDragging = false;
			ui()->releasePointer(*this);
			if (mOnFinished) mOnFinished();
			break;

		default: return;
	}

	event.accept();
	refreshReveal();
}

void ResizeHandle::onHoverChanged() { refreshReveal(); }

void ResizeHandle::refreshReveal() {
	const halnf target = isHovered() || mDragging ? 1.0f : 0.0f;
	if (mReveal.target() == target && !mReveal.isAnimating()) return;

	mReveal.animateTo(target);
	startTicking();
}

void ResizeHandle::onTick(halnf deltaMs) {
	if (!mReveal.advance(deltaMs)) stopTicking();
	requestPaint();
}

void ResizeHandle::paint(Painter& painter) {
	const halnf reveal = mReveal.value();
	if (reveal <= 0.01f) return;

	const Palette& palette = theme().palette;
	const RGBA base = mDragging ? palette.text : palette.mutedText;
	painter.fillRect(lineRect(), { base.r, base.g, base.b, base.a * reveal }, kLineThickness / 2);
}
