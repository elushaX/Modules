#include "DebugManager.hpp"
#include "RootWidget.hpp"
#include "BasicLayout.hpp"

#include "imgui.h"
#include <sstream>

using namespace tp;

DebugManager tp::gDebugWidget;

#define LIST_SIZE { -FLT_MIN, 150 }

bool DebugManager::update(RootWidget* rootWidget, EventHandler& events) {
	mRootWidget = rootWidget;

	events.setEnableKeyEvents(true);
	if (events.isPressed(InputID::D)) mDebug = !mDebug;

	if (mDebug) {
		auto area = rootWidget->mRoot.getAreaT();
		area.size -= { 400, 0 };
		rootWidget->mRoot.setArea(area);

		events.setEnableKeyEvents(true);
		if (events.isPressed(InputID::K)) mDebugStopProcessing = !mDebugStopProcessing;
		if (mDebugStopProcessing) return false;

		if (auto widget = rootWidget->mUpdateManager.mInFocusWidget) {
			if (auto lay = dynamic_cast<BasicLayout*>(widget->getLayout())) {
				if (events.isPressed(InputID::V)) lay->setLayoutPolicy(LayoutPolicy::Vertical);
				if (events.isPressed(InputID::H)) lay->setLayoutPolicy(LayoutPolicy::Horizontal);

				auto sizing = lay->getSizePolicy();

				if (events.isDown(InputID::X)) {
					if (events.isPressed(InputID::S)) lay->setSizePolicy(SizePolicy::Minimal, sizing.y);
					if (events.isPressed(InputID::E)) lay->setSizePolicy(SizePolicy::Expanding, sizing.y);
				}
				if (events.isDown(InputID::Y)) {
					if (events.isPressed(InputID::S)) lay->setSizePolicy(sizing.x, SizePolicy::Minimal);
					if (events.isPressed(InputID::E)) lay->setSizePolicy(sizing.x, SizePolicy::Expanding);
				}
			}
		}

		if (auto breakWidget = rootWidget->mUpdateManager.mInFocusWidget) {
			if (events.isPressed(InputID::P)) mProcBreakpoints.insert(breakWidget);
			if (events.isPressed(InputID::L)) mLayBreakpoints.insert(breakWidget);
		}
	}

	return true;
}

void DebugManager::drawDebug(RootWidget* rootWidget, Canvas& canvas) {
	mRootWidget = rootWidget;

	if (!mDebug) {
		return;
	}

	// ImGui::Checkbox("Draw debug", &mDebug);

	ImGui::SameLine(); ImGui::Text("To Toggle processing press k");

	auto& upd = rootWidget->mUpdateManager;

	recursiveDraw(canvas, &rootWidget->mRoot, { 0, 0 }, 0);

	ImGui::Text("Triggered: %i", (int) upd.mTriggeredWidgets.size());
	ImGui::SameLine(); ImGui::Text("Processing: %i", upd.mDebugWidgetsToProcess);

	ImGui::Checkbox("Stop processing", &mDebugStopProcessing);
	ImGui::SameLine(); ImGui::Checkbox("Force new frames", &mDebugRedrawAlways);
	ImGui::SameLine(); ImGui::Checkbox("Slow-mo", &mSlowMotion);

	if (upd.mInFocusWidget) {
		ImGui::Text("Under cursor");
		{
			if (ImGui::BeginListBox("##under_cursor", LIST_SIZE)) {
				for (auto widget = upd.mInFocusWidget; widget && widget->mParent; widget = widget->mParent) {
					widgetMenu(widget);
				}
				ImGui::EndListBox();
			}
		}
	}

	ImGui::Text("Triggered");
	{
		if (ImGui::BeginListBox("##triggered", LIST_SIZE)) {
			for (auto widget : upd.mTriggeredWidgets) {
				widgetMenu(widget.first);
			}
			ImGui::EndListBox();
		}
	}

	drawLayoutOrder();
}

void DebugManager::recursiveDraw(Canvas& canvas, Widget* active, const Vec2F& pos, int depthOrder) {
	auto area = RectF{ pos, active->getAreaT().size };

	if (active->isUpdate()) {
		RGBA color = { 1, 0, 0, 1 };
		canvas.frame(area, color);
		canvas.text((active->mDebug.id + ":" + std::to_string(depthOrder)).c_str(), area, 22, Canvas::Align::LC, 2, color);
	}

	if (active->mFlags.get(Widget::IN_FOCUS)) {
		if (active->isUpdate()) {
			canvas.circle(pos + active->mDebug.pLocal, 5, active->mDebug.col);
			canvas.circle(active->mDebug.pGlobal, 15, active->mDebug.col);
		}

		RGBA color = { 1, 0, 0, 0.3f };
		canvas.debugCross(area, color);
	}

	int orderIdx = 0;
	for (auto child = active->mDepthOrder.lastNode(); child; child = child->prev) {
		recursiveDraw(canvas, child->data, pos + child->data->getAreaT().pos, orderIdx++);
	}
}

void DebugManager::widgetMenu(Widget* widget) {

	ImGui::PushID(widget);

	if (ImGui::CollapsingHeader((widget->mDebug.id + std::to_string((long) widget)).c_str())) {

		ImGui::Text("trigger reason: %s", widget->mDebug.triggerReason.c_str());

		auto area = widget->getAreaT();
		if (ImGui::InputFloat4("rect", &area.x)) {
			widget->setArea(area);
		}

		auto layout = widget->getLayout();

		ImGui::InputFloat2("min size", &layout->mMinSize.x);
		ImGui::InputFloat2("max size", &layout->mMaxSize.x);

		{
			int sizePolicyX = int(widget->getLayout()->getSizePolicy().x);
			int sizePolicyY = int(widget->getLayout()->getSizePolicy().y);

			if (ImGui::Combo("Size Policy X", &sizePolicyX, "Fixed\0Expanding\0Minimal\0")) {
				widget->setSizePolicy(SizePolicy(sizePolicyX), SizePolicy(sizePolicyY));
			}

			if (ImGui::Combo("Size Policy Y", &sizePolicyY, "Fixed\0Expanding\0Minimal\0")) {
				widget->setSizePolicy(SizePolicy(sizePolicyX), SizePolicy(sizePolicyY));
			}
		}

		if (auto pBasicLayout = dynamic_cast<BasicLayout*>(widget->getLayout())) {
			int policy = int(pBasicLayout->mLayoutPolicy);
			if (ImGui::Combo("Layout", &policy, "Passive\0Vertical\0Horizontal\0")) {
				pBasicLayout->mLayoutPolicy = LayoutPolicy(policy);
			}
		}
	}
	ImGui::PopID();
}

void DebugManager::drawLayoutOrder() {
	ImGui::Text("Layout Processing Order");
	if (ImGui::BeginListBox("##layout_order", LIST_SIZE)) {
		for (auto iter : mRootWidget->mLayoutManager.mLayOrder) {
			ImGui::Text("%i %s (%s)", iter.second, iter.first->mDebug.id.c_str(), std::to_string((long) iter.first).c_str());
		}
		ImGui::EndListBox();
	}
}