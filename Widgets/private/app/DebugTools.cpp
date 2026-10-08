#include "DebugTools.hpp"

using namespace tp;
using namespace tp::ui;

DebugTools::DebugTools(SessionRecorderHost& host, DebugInspector& inspector, Widget& anchor) :
	mInspector(inspector),
	mRecorder(host) {
	mMenu.setDebugName("debug menu");
	mRecording.setAnchor(&anchor);
	addMenuItems(mMenu);
}

DebugTools::~DebugTools() {
	detach();
	mRecorder.cancel();
}

void DebugTools::addMenuItems(Menu& menu) {
	menu.addToggle(
		"Debug UI",
		[this]() { return mInspector.isVisible(); },
		[this]() { mInspector.setVisible(!mInspector.isVisible()); }
	);
	menu.addSeparator();
	mRecording.addMenuItems(menu);
}

void DebugTools::attachTo(MenuBar& bar) {
	if (&bar == mBar) return;
	detach();
	mBar = &bar;
	bar.addTrailingMenu("Debug", mMenu);
	bar.setTrailingWidget(&mRecording.badge);
}

void DebugTools::detach() {
	if (!mBar) return;
	mBar->removeMenu(mMenu);
	if (mBar->trailingWidget() == &mRecording.badge) mBar->setTrailingWidget(nullptr);
	mBar = nullptr;
}
