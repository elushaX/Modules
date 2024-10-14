#include "DebugManager.hpp"
#include "RootWidget.hpp"

#include "imgui.h"

using namespace tp;

bool DebugManager::update(RootWidget* rootWidget, EventHandler& events) {
	if (mDebug) {
		auto area = rootWidget->mRoot->getAreaT();
		area.size -= { 400, 0 };
		rootWidget->mRoot->setArea(area);

		events.setEnableKeyEvents(true);
		if (events.isPressed(InputID::K)) mDebugStopProcessing = !mDebugStopProcessing;
		if (mDebugStopProcessing) return false;
	}

	return true;
}

void DebugManager::drawDebug(RootWidget* rootWidget, Canvas& canvas) {
	ImGui::Checkbox("Draw debug", &mDebug);

	auto& upd = rootWidget->mUpdateManager;

	if (mDebug) {
		recursiveDraw(canvas, rootWidget->mRoot, { 0, 0 }, 0);

		ImGui::Text("Triggered Widgets: %i", (int) upd.mTriggeredWidgets.size());
		ImGui::Text("Widgets To Process: %i", upd.mDebugWidgetsToProcess);

		ImGui::Text("To Toggle processing press k");
		ImGui::Checkbox("Stop processing", &mDebugStopProcessing);
		ImGui::Checkbox("Force new frames", &mDebugRedrawAlways);

		if (ImGui::CollapsingHeader("Triggered")) {
			if (ImGui::BeginListBox("##triggered", { -FLT_MIN, 300 })) {
				for (auto widget : upd.mTriggeredWidgets) {
					widgetMenu(widget.first);
				}
				ImGui::EndListBox();
			}
		}

		if (upd.mInFocusWidget) {
			if (ImGui::CollapsingHeader("Under cursor")) {
				if (ImGui::BeginListBox("##under_cursor", { -FLT_MIN, 300 })) {
					for (auto widget = upd.mInFocusWidget; widget && widget->mParent; widget = widget->mParent) {
						widgetMenu(widget);
					}
					ImGui::EndListBox();
				}
			}
		}
	}
}

void DebugManager::recursiveDraw(Canvas& canvas, Widget* active, const Vec2F& pos, int depthOrder) {
	auto area = RectF{ pos, active->getAreaT().size };

	if (active->isUpdate()) {
		RGBA color = { 1, 0, 0, 1};
		canvas.frame(area, color);
		canvas.text((active->mDebug.id + ":" + std::to_string(depthOrder)).c_str(), area, 22, Canvas::Align::LC, 2, color);
	}

	if (active->mFlags.get(Widget::IN_FOCUS)) {
		if (active->isUpdate()) {
			canvas.circle(pos + active->mDebug.pLocal, 5, active->mDebug.col);
			canvas.circle(active->mDebug.pGlobal, 15, active->mDebug.col);
		}

		RGBA color = { 1, 0, 0, 0.3f};
		canvas.debugCross(area, color);
	}

	int orderIdx = 0;
	for (auto child = active->mDepthOrder.lastNode(); child; child = child->prev) {
		recursiveDraw(canvas, child->data, pos + child->data->getAreaT().pos, orderIdx++);
	}
}

void DebugManager::widgetMenu(Widget* widget) {

	ImGui::PushID(widget);
	if (ImGui::CollapsingHeader(widget->mDebug.id.c_str())) {

		ImGui::Text("trigger reason: %s", widget->mDebug.triggerReason.c_str());

		auto area = widget->getAreaT();
		if (ImGui::InputFloat4("rect", &area.x)) {
			widget->setArea(area);
		}

		// ImGui::InputFloat2("min size", &widget->mMinSize.x);
		// ImGui::InputFloat2("max size", &widget->mMaxSize.x);

		// int sizePolicyX = int(widget->mSizePolicy.x);
		// int sizePolicyY = int(widget->mSizePolicy.y);
		// int layout = int(widget->mLayoutPolicy);

		// if (ImGui::Combo("Size Policy X", &sizePolicyX, "Fixed\0Expanding\0Minimal\0")) {
		// widget->setSizePolicy(SizePolicy(sizePolicyX), SizePolicy(sizePolicyY));
		// }

		// if (ImGui::Combo("Size Policy Y", &sizePolicyY, "Fixed\0Expanding\0Minimal\0")) {
		// widget->setSizePolicy(SizePolicy(sizePolicyX), SizePolicy(sizePolicyY));
		// }

		// if (ImGui::Combo("Layout", &layout, "Passive\0Vertical\0Horizontal\0")) {
		// widget->setLayoutPolicy(LayoutPolicy(layout));
		// }
	}
	ImGui::PopID();
}