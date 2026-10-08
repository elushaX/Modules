#include "FloatingPanel.hpp"

#include "BoxLayout.hpp"
#include "Event.hpp"
#include "StackLayout.hpp"
#include "Theme.hpp"
#include "Ui.hpp"

using namespace tp;
using namespace tp::ui;

namespace {
	constexpr halnf kGripSize = 14;
}

FloatingPanel::FloatingPanel(std::string title) :
	Panel(Surface::Raised),
	mTitle(std::move(title)) {
	setDebugName("floating panel");
	setBorder(true);

	emplaceLayout<ColumnLayout>(0);

	mTitleBar.setSizing(AxisSizing::fill(), AxisSizing::fixed(Theme::standard().metrics.controlHeight));
	mTitleBar.emplaceLayout<RowLayout>().setPadding(Insets::symmetric(8, 0));
	mTitle.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mTitleBar.addChild(mTitle);

	mBody.setSizing(AxisSizing::fill(), AxisSizing::fill());
	mBody.emplaceLayout<StackLayout>().setPadding(Insets::uniform(6));

	addChild(mTitleBar);
	addChild(mBody);
	applySize();
}

void FloatingPanel::setContent(Widget* content) {
	if (mContent) mBody.removeChild(*mContent);

	mContent = content;
	if (mContent) mBody.addChild(*mContent);
}

void FloatingPanel::setPanelSize(Vec2F size) {
	mPanelSize = { size.x > mMinimumSize.x ? size.x : mMinimumSize.x, size.y > mMinimumSize.y ? size.y : mMinimumSize.y };
	applySize();
}

RectF FloatingPanel::titleBarRect() const { return mTitleBar.rect(); }

RectF FloatingPanel::resizeGripRect() const {
	return makeRect(size().x - kGripSize, size().y - kGripSize, kGripSize, kGripSize);
}

void FloatingPanel::onEvent(Event& event) {
	switch (event.type()) {
		case Event::Type::PointerDown:
			{
				if (event.button() != PointerButton::Primary) return;

				const Vec2F local = event.position();
				if (contains(resizeGripRect(), local)) mInteraction = Interaction::Resizing;
				else if (contains(titleBarRect(), local)) mInteraction = Interaction::Moving;
				else return;

				mGrabOffset = mInteraction == Interaction::Moving ? local : local - size();
				bringToFront();
				ui()->capturePointer(*this);
				event.accept();
				break;
			}

		case Event::Type::PointerMove:
			{
				if (mInteraction == Interaction::None || !parent()) return;

				const Vec2F inParent = event.windowPosition() - parent()->windowPosition();
				if (mInteraction == Interaction::Moving) requestPosition(inParent - mGrabOffset);
				else setPanelSize(event.position() - mGrabOffset);
				event.accept();
				break;
			}

		case Event::Type::PointerUp:
			if (mInteraction == Interaction::None) return;
			mInteraction = Interaction::None;
			ui()->releasePointer(*this);
			event.accept();
			break;

		default: return;
	}

	requestPaint();
}

void FloatingPanel::paintOverlay(Painter& painter) {
	const Theme& style = theme();
	const RectF grip = resizeGripRect();
	const RGBA color = isResizing() ? style.palette.accent : style.palette.mutedText;

	painter.line(grip.pos + Vec2F(grip.size.x, 4.0f), grip.pos + Vec2F(4.0f, grip.size.y), color, 1);
	painter.line(grip.pos + Vec2F(grip.size.x, 9.0f), grip.pos + Vec2F(9.0f, grip.size.y), color, 1);
}

void FloatingPanel::applySize() { setSizing(AxisSizing::preferred(mPanelSize.x), AxisSizing::preferred(mPanelSize.y)); }
