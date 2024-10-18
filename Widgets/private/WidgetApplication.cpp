#include "WidgetApplication.hpp"

#include "imgui.h"

using namespace tp;

void WidgetApplication::debugUI() {
	// pass
}

void WidgetApplication::setRoot(Widget* widget) {
	mRootWidget.setRootWidget(widget);

	tp::RootWidget::setWidgetArea(*widget, {{}, mWindow->getSize()});
}

void WidgetApplication::processFrame(EventHandler* eventHandler, halnf deltaTime) {
	const auto area = RectF({ 0, 0 }, mWindow->getSize());

	gDebugWidget.update(&mRootWidget, *eventHandler);

	if (gDebugWidget.isDebug()) {
		mGuiArea = area.splitByFactorHL(mDebugSplitFactor);
		mDebugArea = area.splitByFactorHR(mDebugSplitFactor);
	} else {
		mGuiArea = area;
	}

	if (gDebugWidget.isFrozen()) return;

	Timer timer;
	mRootWidget.processFrame(eventHandler, mGuiArea);
	gDebugWidget.mProcTime.addSample(timer.timePassed());
}

void WidgetApplication::drawFrame(Canvas* canvas) {
	Timer timer;
	mRootWidget.drawFrame(*canvas);
	gDebugWidget.mDrawTime.addSample(timer.timePassed());

	if (gDebugWidget.isDebug()) {
		ImGui::SetNextWindowPos(ImVec2(mDebugArea.x, mDebugArea.y));
		ImGui::SetNextWindowSize(ImVec2(mDebugArea.z, mDebugArea.w));

		if (ImGui::Begin(
					"Existing Window Name",
					nullptr,
					ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoDecoration
				)) {

			drawDebug();
			debugUI();

			gDebugWidget.drawDebug(&mRootWidget, *canvas);
		}
		ImGui::End();
	}
}

bool WidgetApplication::forceNewFrame() {
	return mRootWidget.needsUpdate();
}