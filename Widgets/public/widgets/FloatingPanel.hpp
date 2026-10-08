#pragma once

#include "Containers.hpp"
#include "Label.hpp"

#include <string>

namespace tp::ui {

	class FloatingPanel : public Panel {
	public:
		explicit FloatingPanel(std::string title = {});

		void setTitle(std::string title) { mTitle.setText(std::move(title)); }
		[[nodiscard]] const std::string& title() const { return mTitle.text(); }

		void setContent(Widget* content);
		[[nodiscard]] Widget* content() const { return mContent; }

		void setPanelSize(Vec2F size);
		[[nodiscard]] Vec2F panelSize() const { return mPanelSize; }
		void setMinimumPanelSize(Vec2F size) { mMinimumSize = size; }

		[[nodiscard]] bool isMoving() const { return mInteraction == Interaction::Moving; }
		[[nodiscard]] bool isResizing() const { return mInteraction == Interaction::Resizing; }

		[[nodiscard]] RectF titleBarRect() const;
		[[nodiscard]] RectF resizeGripRect() const;

	protected:
		void onEvent(Event& event) override;
		void paintOverlay(Painter& painter) override;

	private:
		enum class Interaction { None, Moving, Resizing };

		void applySize();

		Label mTitle;
		Widget mTitleBar;
		Widget mBody;
		Widget* mContent = nullptr;

		Vec2F mPanelSize = { 240, 200 };
		Vec2F mMinimumSize = { 120, 80 };

		Interaction mInteraction = Interaction::None;
		Vec2F mGrabOffset = { 0, 0 };
	};
}
