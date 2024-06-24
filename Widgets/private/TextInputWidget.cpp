#include "TextInputWidget.hpp"

#include "imgui.h"
#include "imgui_internal.h"

using namespace tp;

TextInputWidget::TextInputWidget() = default;

void TextInputWidget::eventDraw(Canvas&) {
	nChanged = false;

	const auto col = mAccentColor;
	const auto colSel = mHoveredColor;

	ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = { col.r, col.g, col.b, col.a };
	ImGui::GetStyle().Colors[ImGuiCol_TextSelectedBg] = { colSel.r, colSel.g, colSel.b, colSel.a };

	ImGui::SetNextWindowPos({ this->mArea.x, this->mArea.y });
	ImGui::SetNextWindowSize({ this->mArea.z, this->mArea.w });
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, mRounding * 1.5f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { mPadding, mPadding });

	// ImGui::PushID((int) alni(this));
	ImGui::Begin(
		mId.c_str(),
		0,
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoResize
	);

	if (mMultiline) {
		if (ImGui::InputTextMultiline("input", mBuff, mMaxBufferSize, { this->mArea.z, this->mArea.w })) {
			mValue = mBuff;
			nChanged = true;
		}
	} else {
		if (ImGui::InputTextEx("input", mId.c_str(), mBuff, mMaxBufferSize, { this->mArea.z, this->mArea.w }, 0)) {
			mValue = mBuff;
			nChanged = true;
		}
	}

	ImGui::End();
	ImGui::PopStyleVar(3);
}

void TextInputWidget::eventUpdateConfiguration(WidgetManager& wm) {
	wm.setActiveId("TextInput");

	mAccentColor = wm.getColor("Accent", "Accent");
	mBaseColor = wm.getColor("Base", "Base");
	mRounding = wm.getNumber("Rounding", "Rounding");
	mHoveredColor = wm.getColor("Hovered", "Accent");
	mPadding = wm.getNumber("Padding", "Padding");
}
