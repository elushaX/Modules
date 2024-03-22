#pragma once

#include "WidgetBase.hpp"

#include "imgui.h"
#include "imgui_internal.h"

namespace tp {
	template <typename Events, typename Canvas>
	class TextInputWidget : public Widget<Events, Canvas> {
	public:
		TextInputWidget() {
			this->createConfig("TextInput");
			this->addColor("Accent", "Accent");
			this->addColor("Base", "Base");
			this->addValue("Rounding", "Rounding");
			this->addColor("Hovered", "Interaction");
			this->addValue("Padding", "Padding");
		}

		void proc(const Events& events, const tp::RectF& areaParent, const tp::RectF& aArea) override {
			this->mArea = aArea;
			this->mVisible = areaParent.isOverlap(aArea);
			if (!this->mVisible) return;
		}

		void draw(Canvas& canvas) override {
			if (!this->mVisible) return;

			nChanged = false;

			const auto col = this->getColor("Accent");
			const auto colSel = this->getColor("Hovered");

			ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = { col.r, col.g, col.b, col.a };
			ImGui::GetStyle().Colors[ImGuiCol_TextSelectedBg] = { colSel.r, colSel.g, colSel.b, colSel.a };

			ImGui::SetNextWindowPos({ this->mArea.x, this->mArea.y });
			ImGui::SetNextWindowSize({ this->mArea.z, this->mArea.w });
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, this->getValue("Rounding") * 1.5f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { this->getValue("Padding"), this->getValue("Padding") });

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

		enum { mMaxBufferSize = 512 };
		char mBuff[mMaxBufferSize] = "";
		bool nChanged = false;
		std::string mValue;
		std::string mId = "id";
		bool mMultiline = false;
	};
}