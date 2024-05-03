#pragma once

#include "WidgetBase.hpp"

#include "imgui.h"
#include "imgui_internal.h"

namespace tp {
	template <typename Events, typename Canvas>
	class TextInputWidget : public Widget<Events, Canvas> {
	public:
		TextInputWidget() { this->mId = "TextInput"; }

		void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) override {
			this->mArea = aArea;
			this->mVisible = areaParent.isOverlap(aArea);
			if (!this->mVisible) return;
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;

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

	public:
		void setupConfig(WidgetManager& wm) {
			if (!wm.createWidgetConfig(this->mId)) return;
			wm.addReference(this->mId, "Accent", "Accent");
			wm.addReference(this->mId, "Base", "Base");
			wm.addReference(this->mId, "Rounding", "Rounding");
			wm.addReference(this->mId, "Hovered", "Accent");
			wm.addReference(this->mId, "Padding", "Padding");
		}

		void updateConfigCache(const WidgetManager& wm) override {
			mAccentColor = wm.getColor(this->mId, "Accent");
			mBaseColor = wm.getColor(this->mId, "Base");
			mHoveredColor = wm.getColor(this->mId, "Hovered");
			mRounding = wm.getNumber(this->mId, "Rounding");
			mPadding = wm.getNumber(this->mId, "Padding");
		}

	public:
		enum { mMaxBufferSize = 512 };
		char mBuff[mMaxBufferSize] = "";
		bool nChanged = false;
		std::string mValue;
		std::string mId = "id";
		bool mMultiline = false;

		RGBA mAccentColor;
		RGBA mHoveredColor;
		RGBA mBaseColor;
		halnf mRounding = 0;
		halnf mPadding = 0;
	};
}