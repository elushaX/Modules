#include "ApplicationFrame.hpp"

#include "BoxLayout.hpp"

using namespace tp;
using namespace tp::ui;

ApplicationFrame::ApplicationFrame() :
	Panel(Surface::Window) {
	setDebugName("application frame");
	setSizing(AxisSizing::fill(), AxisSizing::fill());
	setCornerRadius(0);
	emplaceLayout<ColumnLayout>();
	addChild(menuBar);
}

void ApplicationFrame::setContent(Widget* content) {
	if (content == mContent) return;
	if (mContent) removeChild(*mContent);

	mContent = content;
	if (!mContent) return;

	mContent->setSizing(AxisSizing::fill(), AxisSizing::fill());
	addChild(*mContent);
}

void ApplicationFrame::setMenuBarVisible(bool visible) { menuBar.setVisible(visible); }
